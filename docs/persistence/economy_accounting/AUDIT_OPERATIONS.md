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

Native money and item revisions are exact JSON integers in 0..UINT64_MAX,
matching their unsigned native schema and wire fields. Boolean, negative or
larger values are malformed evidence. This differs from signed denomination
and copper-total ranges. Item events advance one revision; money effects retain
their native before/after revision chain. Origins, current authority and retained
creation/retirement roots must agree across the complete unsigned range.

Each parsed denomination vector must have signed 64-bit fields and a checked
signed 64-bit copper total using weights 1/10/100/1000. This includes native
holdings, opening origins and account-effect before/after vectors, even if they
match each other and the postings balance. An overflowing vector is malformed
evidence: the CLI refuses with status 2 and copper overflow, including with
`--limit 0`; it never wraps values or changes the input to make it reconcile.

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

Persisted ship coffers are separate raw candidates: `native.ship_coffers`
contains only `ship_id` and nullable signed-INT `copper`, with no owner alias,
lineage, account key or fabricated revision. Every `ships` row is included,
including zero/NULL/negative values. `native.ship_coffer_coverage` records rows,
positive/zero/unknown/invalid rows and missing-revision rows. The independent
reconciler validates identities, exact value bounds and uniqueness, recomputes
these counts, and reports `unsupported_native_ship_coffer` and
`missing_ship_coffer_revision` for each row; NULL also reports
`unknown_native_ship_coffer`, and negative values `invalid_native_ship_coffer`.
A partial SQL snapshot missing this collection reports
`missing_ship_coffer_coverage`. Counts remain accurate with detail limit zero.
The exporter requires the `ships` source to be present and InnoDB in the same
read-only consistent cut. These candidates are outside mapped `native.holdings`;
ship lifetimes, origins, revisions, gameplay writers and runtime-only funds
remain unqualified. This evidence retains `complete: false`.

Its `source_claims` collection covers nonbaseline claims across the selected
lineage, including claims from other epochs. The SQL rows carry their owning
operation's lineage, epoch, source and outcome so the reconciler can distinguish
a valid earlier-epoch claim from an orphan. `source_claim_coverage` counts all
committed nonbaseline operations in that lineage with a nonnull source event,
missing exact claims and source values reused by multiple such operations.
The reconciler emits coded counts for the latter two. This collection covers
only operations with a nonnull source identity; baseline source claims remain
outside this collection.

`source_event_policy_coverage` separately counts committed nonbaseline
operations across the selected lineage whose registry policy requires a source
event, including operations from prior epochs. Missing source identities produce
a coded exception. This catches missing required events without loading every
historical operation into the current-epoch evidence tables; it does not prove
that a present source identity describes the correct gameplay event.

- `native.holdings`: each admitted **ordinary** account key, current four-value
  denomination vector and native revision. Wallet and bank keys use retained
  mapping lifetime IDs; a pile key uses its UID. Auction escrow includes held
  bids for open listings and removed listings that still have a winning bidder,
  and is zero when there is no winning bidder. Pending claims and treasuries
  require real durable lifetime identities and native balances.
- `native.retired_mappings`: retained SQL mapping lifetimes with their retiring
  root metadata. The linked `native.retirement_roots` records retain the root
  reason, complete account effects and postings. Reconciliation checks the
  registry's per-reason `retires_account_kinds` policy along with the committed
  same-lineage root, posting agreement and zero terminal balance. A
  current-epoch row must also match its opening account origin and have no
  native holding.
- `native.lineage_uid_references`: every retained item reference for the selected
  lineage across epochs, with its referenced legacy ownership event. Companion
  root counts are reconciled against each operation's declared `item_event_count`.
  The records also seed the UID event census, so historical referenced UIDs stay
  in scope after leaving native state.
- `native.uid_history_events`: every ownership-ledger event after the retained
  opening revision for a UID anchored by an origin, reference or native row.
  The reconciler checks exact reference status, the per-UID revision chain and
  the final native position. Events without an accounting reference remain
  visible with `referenced: false` and produce exceptions.
- `native.uid_scope_coverage` and `native.unanchored_ownership_uids`: a
  database-wide count of distinct ownership-ledger UIDs and the bounded IDs
  absent from this snapshot's origins, references and native rows. Any such UID
  is an explicit unassigned-history exception; this count cannot assign it to a
  particular lineage.
- `native.lineage_realized_prices` and `native.realized_price_coverage`: all
  shop buy/sell roots across the selected lineage, including outcome and the
  persisted copper price. SQL schema migration 0046 adds the nullable field;
  committed shop buys/sales retain their frozen command price in the same
  operation transaction. The exporter detects older schemas or missing
  committed prices and reports explicit exceptions. It does not infer a price
  from appraisal or postings.
- `account_origins`: the same version-1 40-byte account key (lowercase hex), initial
  vector/revision, and `origin: baseline` from an exact EAB1 witness or
  `origin: creation` with zero vector and revision zero when a committed first
  effect matches the retained mapping's creating operation. This inference is
  limited to mapped ordinary accounts with an exact zero/revision-zero opening;
  unsupported or unknown openings are reported, never silently treated as zero.
  A retired mapping carries `retired_by` with the
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
  The SQL exporter reads it from the persisted field; estimated inventory values
  do not belong here.
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
source claims, inbox receipts, pending auction-claim source allocations, and
mapped SQL wallet, bank, coin-pile,
auction-escrow, pending-claim and shop-treasury rows, and current UID positions
in **one** read-only consistent transaction. It writes a bounded
version-1 diagnostic snapshot with `backend: sql_partial`, `complete: false`
and a `capture_gaps` list. Pass that file to the reconciler to investigate
observed exceptions; its nonzero `evidence_loss` result is mandatory even when
all captured rows agree. The exporter omits baseline root effects because the
verified EAB1 witnesses provide their terminal opening origins. It exports
retained item references and exact ownership-event links across the selected
lineage, then exports ledger events after each opening revision for UIDs anchored
by those references, baseline origins, or any current native UID row. The
reconciler checks their exact reference status, revision chain and final native
position. Events without an exact operation, event-index and UID reference are
reported. This detects unreferenced activity for every currently stored or
historically referenced UID, but cannot certify UIDs absent from native state
and all references; a database-wide UID census now counts and identifies those
unanchored histories, but cannot assign them to a lineage. Retained SQL mapping
retirements and their complete root reasons, effects and postings are exported
across epochs. Reconciliation checks root identity, authorized reason/account
kind, balanced value, effect/posting agreement and zero terminal balance. A
current-epoch retirement is also checked against its opening origin
and for absence of a native holding; prior-epoch native rows remain visible as
unmapped candidates. Its mapping census can flag orphan candidates, duplicate
active links and dangling links; it does not resolve legacy admission or
lineage ownership. Retained mappings with a locator kind that does not match
the account kind raise `invalid_native_<kind>_mapping`; invalid active mappings
do not make a native row count as mapped. Unsupported mapping account kinds
raise `invalid_native_mapping_kind`. Coin piles use their
bounded item payload and preserve UID, owner, revision and state. Open auction
escrow and removed auction escrow that still has a winning bidder, aggregate
pending claims and non-null shop cash are exported as copper balances with their
available native revisions. Auction escrow is zero without a winning bidder. A
missing claim row is treated
as zero only when the mapped player still exists. Missing coin payloads and null
treasury cash remain invalid rather than being guessed. For each mapped holding
with a retained opening origin, the reconciler compares the current native
balance and revision with the ordered posting history; the disposable fixture
also proves that changing one pile denomination raises `stale_native_balance`.
The export includes pending claim source allocations and flags missing or
mismatched claim lifetime mappings. It verifies each source root is committed
in the same lineage, has a balanced posting set and credits the mapped claim by
the allocated amount; open source totals must match the native claim balance.
Consumed-source debit linkage is checked, but completeness across legacy claim
writers and their logical source-event attribution are still unproven. Linked
post-baseline create events now provide a creation origin; an unreferenced event
for a tracked UID is reported as an exception, while origins for events outside
that UID set remain unknown. The export also does not prove complete coin-pile
mapping, escrow, claim or treasury lifecycle/origin semantics, and baseline
source claims. A partial snapshot
cannot authorize activation or count as the Plan 5 full SQL audit acceptance.

The guarded disposable runner
`tests/async/run_economic_sql_audit_snapshot_mysql.py` inserts a baseline and a
committed wallet-to-bank root into a minimal InnoDB schema, checks the
`SELECT`-only CLI export and reconciler refusal, changes a native wallet from
another connection during the audit cut to verify repeatable-read isolation,
and injects a missing posting, stale native wallet balance, unmapped native
wallet, duplicate wallet mapping and dangling bank mapping. It also exports a
live coin pile and a retired coin pile, verifies their exact UID and denomination
vectors, refuses a malformed payload, changes a pile denomination and verifies
the reconciler detects the stale holding, and checks mapped auction escrow,
pending claim and shop-treasury balances. A wallet mapped to another lineage is
excluded from the unmapped count. A prior-epoch claim is accepted, while a
missing cross-epoch claim, reused source and orphan claim produce exceptions.
The fixture also verifies a prior-epoch pending-claim credit and consumer debit,
rejects changed source and consumer effects, checks open allocations against
the aggregate, derives origins for one linked post-baseline UID creation and a
newly mapped wallet, reports an unreferenced destruction event for a tracked
UID, exports earlier-epoch retirement root evidence, and rejects a retirement
whose terminal balance is nonzero. It counts a missing required source event on
a prior-epoch operation under the current registry policy, verifies a linked
prior-epoch UID creation, reconciles its full two-event revision chain, finds
an unreferenced destroy event after it, and verifies realized-price export and
missing-column reporting. The fixture also injects an ownership
UID with no selected origin, reference or native row and verifies the scope
census raises an unanchored-history exception.
The current full fixture ran on MariaDB 10.11. MySQL, fresh/upgrade schema and
playable authority qualification remain separate release gates.

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

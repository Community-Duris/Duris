# Economy accounting audit operations

The read-only reconciler is `scripts/reconcile_economy_accounting.py`. It accepts
one bounded, consistent snapshot of native authority and immutable evidence. It
never connects to a writer, posts an adjustment, or changes a balance or UID.
Exceptions mean **pause the affected writer/epoch and investigate**; a zero
exception count is meaningful only when the snapshot is complete and all route
coverage gates are independently qualified.

The bounded `--view provenance --uid <UID>` query combines selected-epoch
ownership events, captured lineage UID history, and captured unattributed UID
events. It orders by revision, operation ID and event index, counts identical
projections once, and retains conflicting positions for investigation. Its
`coverage` object names the lineage and selected epoch, carries the input's
`complete` and `quiescent` flags, and says whether the two history collections
are available. These flags describe captured evidence, not a release
certification. SQL lineage history starts after each retained opening revision;
history before that opening and absent stores are not reconstructed. An empty
result does not prove a UID never existed. Output remains ID-only and bounded;
`--limit 0` retains the full count and coverage while omitting row details.

Provenance matches only captured integer UIDs. A float, boolean, string or null
UID does not match an integer query, even when its numeric value compares
equal. The reader does not convert that representation into a valid UID.
Omitting such a row preserves the whole audit's exception count and CLI exit
status; the filtered result does not certify the omitted evidence.

Run `scripts/economic_sql_canonical_audit.py` with explicit SQL connection
arguments and a SELECT-only account to authenticate original retained EAI1/EAP1
capsules and their SQL projections. For example:

```sh
python3 scripts/economic_sql_canonical_audit.py \
  --host 127.0.0.1 --user accounting_audit --database duris \
  --password-env ACCOUNTING_AUDIT_PASSWORD
```

The command checks every retained root in the database, including inactive
epochs and other lineages, in one read-only repeatable-read transaction. It
always rolls back. Its independent decoder compares original metadata, hashes,
counts, account effects, posting event/line indices, child derivation facts,
item reference event/line indices and legacy custody projections. It requires
InnoDB sources, including `economic_accounting_source_claim`, and refuses
collections above 100,000 roots or source claims, 32 MiB of canonical capsules,
or the per-query row/byte budget. Every committed root with a source identity
requires its exact lineage/source/operation/success claim; orphan, foreign,
changed, duplicate or rejected-root claims refuse across the entire database.
Roots with no source identity and rejected roots require no source claim. Orphan details and
details attached to rejected roots also refuse. It never repairs a discrepancy.

At canonical 0062, this check also reads retained pending-claim source lots and
partial-consumption rows, with a 100,000-row bound per collection and 256-row
primary-key pages. It binds original lot amounts and whole/partial debits to
the original decoded root effects, requires exact source/mapping/PID identities
and committed references, and refuses missing, extra, mixed or overdrawn
allocations. Fully consumed sources retain their original positive amounts.
Declared version1 opening metadata requires exact canonical holding slots and
counts. The restore and origin readers bind it to the retained V2 lifecycle
request and committed genesis receipt, independently recompute the original
PID/amount/revision digests, and check positive source slots. Zero original
claims are authenticated without inventing a positive source row. A retained
new-policy parent prevents clearing the marker from downgrading that opening.
Historical NULL metadata without that parent retains unknown coverage. See
[the original opening qualification](PLAN5_ORIGINAL_OPENING_QUALIFICATION_2026-10-06.md).
The report's `retained_pending_claim_allocations: verified` covers these
retained facts; it does not qualify claim producers or complete source capture.
The saved snapshot exporter/reconciler also reads whole and partial allocations
and compares their remaining amounts with native claim balances. Its 64-pair
source batches count only the exact operation/account pairs requested by that
batch; genuine duplicate rows still refuse. See
[the combined reader qualification](PLAN5_PRIMARY_PAIR_COMBINED_QUALIFICATION_2026-10-06.md).
Source capture and release remain unqualified; a successful check never
authorizes correction of a finding.

Status 0 emits a small JSON report with database scope and verified root/byte
counts. A discrepancy or missing/oversized source emits no report, prints a
fixed diagnostic refusal on stderr, and exits 2. Capsule and command bodies,
aliases and passwords are absent from that output. The JSON explicitly leaves
complete command/receipt authentication, source capture and release
qualification false. A successful check supplements the partial snapshot
exporter: a saved version-1 projection retains original EAP1 plans and omits
opaque EAI1 intent facts. Its reconciler authenticates projected plan fields
against those retained plans; the canonical SQL check additionally authenticates
the retained intent and its SQL binding. Capture the two checks under the release's
quiescence procedure; independent runs do not constitute one combined cut.

For repeated SQL canonical-root checks, add `--progress-path` pointing to a
protected local file. Its parent directory must already exist. The maintained
Linux/POSIX CLI uses an exclusive OS lock and writes a mode-0600 checkpoint by
fsync, atomic rename, and directory fsync. A killed process releases its lock;
a page interrupted before checkpoint publication is retried. Malformed,
oversized, foreign-target, symlinked, or unprotected checkpoints refuse.

```sh
python3 scripts/economic_sql_canonical_audit.py \
  --host 127.0.0.1 --user accounting_audit --database duris \
  --password-env ACCOUNTING_AUDIT_PASSWORD \
  --progress-path /protected/audit/canonical-progress.json --page-roots 2
```

Each invocation checks at most two candidate operation IDs in one read-only consistent snapshot
and rolls back before publishing local progress. It bounds each projection to
8,192 rows, the page to 1,024 SELECTs and 32 MiB of returned text, and admission
of additional queries to 30 seconds. The connection retains its 30-second SQL
read timeout. These are component bounds, not game-loop or release-host budgets.
The sweep freezes its upper ID at the start, finishes that finite range even
while higher IDs arrive, then starts again at the lowest ID. IDs schedule
reads; they never certify commit order. A lower-ID transaction committed behind
the cursor is eligible on the next sweep. Every page has one read view; the
whole sweep combines different read views and always reports `complete=false`.

The candidate range merges IDs from the root table, account effects, coin postings,
children, item references, source claims and baseline witnesses. Each source uses
its existing operation-ID-leading index and direct single-ID seeks. Repeated
details for one operation do not consume additional candidate slots; no whole-table
aggregation is needed. The ceiling covers the same seven sources, so a retained
detail beyond the largest surviving root, or with no surviving roots, is scheduled.
A missing nonzero parent root reports `restore_economic_orphan_root_mismatch`;
the audit leaves the evidence unchanged. `candidate_source_count` records seven
sources and `unattached_root_ids` counts these missing parents on this page.

Version-1 checkpoints remain readable. An already started range finishes under
its saved ceiling, and the following sweep captures the expanded range. Existing
`examined_roots`, `sweep_rows` and `total_rows` fields count scheduled candidate IDs;
they are not counts of surviving stored roots. Sticky findings and inexact backlog
retain their existing meanings. Composite-key reservations and controls, native
holdings and other evidence are outside this candidate enumeration; orphan coverage
still remains incomplete.

This mode reuses the full reader's original EAI1/EAP1 metadata, digest, count,
effect, posting, child, item-reference and custody checks. It also checks each
root's lifecycle namespace and exact source claim. Selected committed baseline
roots additionally authenticate their original EAB1/EAB2 witness, exact
reservations, inbox command/fence binding, successful receipt, versioned
claim-origin policy/identity, and absence of native mutation effects. Reservation
reads use pages of at most 257 rows, so a valid 9,071-reservation witness fits the
existing projection bound. Other roots must have no baseline witness or
reservations; rejected roots must have no details.

Each selected baseline witness also checks its book control revision and terminal
operation in the same read view. An indexed projection reads at most the
predecessor, successor and terminal revisions, refusing gaps, foreign or rejected
neighbour roots and a mismatched terminal as `restore_economic_baseline_book_mismatch`.
These local checks do not enumerate controls with no retained roots or prove a
consistent whole-book cut across separate pages. Full quiescent comparison remains
required; page reports retain incomplete whole-store coverage.

The independent canonical reader checks that observed numeric source columns
use SQL integer storage before reading JSON projections. MySQL/MariaDB can
render integral DOUBLE or DECIMAL values as JSON integers; that conversion
cannot authenticate the original representation. Such altered storage is
reported as `restore_economic_canonical_storage_mismatch`, even when projected
values equal the retained plan. This check also applies to the full restore
reader. It does not replace complete migration/runtime schema qualification.

The report counts authenticated baseline roots and retained NULL claim-origin
markers separately. Historical NULL admission times and claim-origin markers
stay unknown. This mode does not authenticate the entire baseline book,
pending-claim consumption allocations, orphan evidence, complete command
receipts, or current native holdings. Those whole-store coverage fields remain
false, including after an empty or completed range.
The existing full-database check remains necessary under release quiescence.
Flatfile resumable scans and complete reconciliation remain separate gates.

Routine output has aggregate counts, diagnostic codes, page resource metrics,
sweep age and time since the last completed range. Backlog is a lower bound
from the same page's extra key, explicitly inexact; it excludes unseen late
commits and new higher IDs. The protected checkpoint retains at most 32
operation-ID/diagnostic observations and a sticky truncation flag. Subsequent
clean pages do not erase earlier findings or make the CLI report clearance.
Status 1 means retained findings; status 2 means refusal without advancing the
checkpoint. Status 0 means this partial page completed without retained
findings. Local cursor target binding is not a database incarnation or trusted
capture seal. Checkpoint reuse after restore cannot establish historical
coverage, native authority, activation or release readiness.

Every non-exception view includes the whole audited input's `coverage` object:
`lineage`, `selected_epoch`, `complete`, `quiescent`, and `exception_count`.
This includes unfiltered holdings, supply, prices, routes and provenance. The
exception count is global and remains visible in JSON at limits 0/1/100 even
when the view has no matching rows. Existing filtered holdings, provenance and
operation scope fields remain present. The CLI exit status still reflects the
whole audit. Coverage describes the supplied snapshot; it does not qualify the
writer matrix or assert that an empty price/provenance view exhausts history.

The `--view supply` totals use captured postings attached to exactly one
committed root identity, with exactly one posting at its line identity and one
account effect at the referenced account index. Rejected or unknown outcomes
and duplicate root/effect/line identities contribute no supply rows, including
exact repeats; their audit findings remain in the global exception count and
CLI exit status. Export order cannot choose an outcome, account kind or amount
for ambiguous evidence. Distinct posting lines may still aggregate against the
same account. This rule applies to issuance, sinks, opening equity and
restitution. Totals remain selected-epoch evidence, grouped by account kind and
reason, with the full count retained at `--limit 0`. Other discrepancies in a
committed root remain audit exceptions: a displayed total does not certify that
root, its policy or its native effect. See
[the exact outcome qualification](PLAN5_SUPPLY_OUTCOME_VIEW_QUALIFICATION_2026-10-06.md).
Duplicate projection qualification is recorded in
[the unique evidence report](PLAN5_SUPPLY_EVIDENCE_QUALIFICATION_2026-10-06.md).

The bounded `--view prices` query combines selected-epoch roots with captured
`native.lineage_realized_prices`. Each row contains only `epoch`,
`operation_id`, numeric `reason`, and persisted `price_copper`. It includes
committed roots with exact integer prices in 0..INT64_MAX, including zero;
rejected/unknown outcomes and invalid or missing prices are omitted from the
price rows while their audit findings remain in the global exception count.
Exact projections are counted once; conflicting projections remain visible.
Rows sort lexically by epoch and operation ID, then numerically by reason and
price; this order does not claim epoch chronology. The row limit is applied
after deduplication and the full count remains available at `--limit 0`.

Price coverage adds `lineage_history_available` and `realized_price_coverage`.
History is available only when both retained rows and their coverage were
captured. The coverage object preserves the exporter's `column_available`,
`candidate_rows`, and `missing_price_rows`; it is `null` when unavailable.
Candidate rows count captured roots before filtering and deduplication, so they
can differ from the displayed price count. Existing snapshots without the
paired history fields continue to display selected-epoch prices. An empty
captured history, unavailable history, missing price column, and missing
committed price remain distinguishable. These fields describe the supplied
cut; they neither certify complete economic history nor authorize a repair.

Use `--view operation --operation-id <32 lowercase hex digits>` to inspect an
exact root ID in an existing export. The bounded, ID-only records include root
metadata, account effects, postings, child IDs, item references, receipts,
captured source claims and matching database-wide orphan-detail markers.
`count` counts all matching records; `record_counts` reports each collection
before truncation, including zero roots and duplicate roots. The limit applies
to the entire record list, with root metadata first. Aliases, command/result
payloads and canonical blobs are omitted. Duplicate evidence remains visible.
The coverage object carries the selected lineage/epoch, input completeness and
quiescence, and whole-audit exception count. Root capture is selected-epoch
scope; source claims and orphan markers may come from wider captured scopes.
A zero or rootless result does not prove the operation is absent outside this
export. Use another consistent cut for the relevant epoch when needed.

Use `--view holdings --account-key <80 lowercase hex digits>` to select one
exact version-1 account key from captured native holdings. Its coverage names
that key and the captured-native-holdings scope. An absent holding is not proof
that the account never existed or that its retained effects can be discarded.
An invalid operation ID/account key, either filter used with an inappropriate
view, or a limit outside 0..100 refuses with status 2. Both queries preserve the whole audit status:
filtering does not turn an incomplete cut or an unrelated discrepancy into an
all-clear. `--limit 0` preserves counts and coverage without detail rows.

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

Every committed root also requires `canonical_plan`, the lowercase hex of its
original retained EAP1 bytes, and `plan_digest`, `intent_digest`, and
`domain_digest` as lowercase SHA-256 hex. Its metadata includes
`original_operation_id` (null when absent), `accounting_version`, `writer_id`,
`policy_version`, `compiler_version`, `actor_kind`, and `actor_id`, together with
the existing lineage, epoch, operation ID, reason, and source event. Counts
include `before_witness_count` and `after_witness_count`. Missing original plans
produce `missing_original_plan` for every backend, including synthetic inputs;
an internally balanced projection does not supply this proof. Old exports must
be recaptured from retained authority rather than resealed from their projected
rows. Rejected roots must have no canonical plan or plan digest.

The independent reader decodes EAP1 without invoking a mutation codec. It checks
root metadata, counts and digests, then compares original account effects,
posting event/account/child links, child IDs and derivation facts, and item
event/child/UID/revision links. Posting rows preserve `event_index`; item
references preserve `line_index`. Linked ownership events preserve `from_owner`,
`from_equipment_slot`, and `to_equipment_slot` as exact numeric facts. Original
plan custody is compared with the linked ledger's source/destination owners,
root, parent, UID revision, and equipment slots. Coherent rewrites of projected
child derivations or reassignment of posting/item children produce specific
`original_plan_*_mismatch` findings. These checks retain all earlier semantic
findings and never change the supplied snapshot.

Native item positions and both retained UID-history projections also preserve
equipment slots. Reconciliation compares the current slot with its authenticated
opening and final ledger slot, and verifies every recorded from/to transition.
Slot-only drift produces `stale_native_item`; a discontinuity produces
`broken_item_equipment_history`. A combined position/slot mismatch counts one
stale item. Slots must be exact unsigned16 integers; equipped openings/current
items must be live, have no parent, and belong to a player or native mobile;
mobile slots are limited to43. Malformed history slots produce
`invalid_item_equipment_slot` without masking original-capsule mismatch findings.

EAB1 equipment omission remains unknown. When other projections record a slot,
an omitted required opening/history/current slot produces one
`missing_item_equipment_evidence` per UID. No omitted historical slot is filled
with zero. A proven revision0 absent creation origin has slot0. Older snapshots
with all equipment projections omitted remain readable at their original scope;
they cannot supply complete equipment proof. Bounded UID provenance includes
valid numeric from/to slots and excludes personal aliases and malformed slots.

SQL ownership actions retain compound producer semantics. An explicit generic
creation/destruction reason keeps its original action so contradictory endpoints
still produce existing findings. Other reasons classify a destruction-owner
endpoint as `destroy`, or a system-owner source at UID revision1 as `create`;
remaining events stay `move`. This applies to selected roots, linked references,
historical UID events and unattributed history. Craft, quest and collector
reasons remain unchanged in retained authority. Creation-origin inference still
requires a committed revision0-to1 creation; rejected and unknown outcomes cannot
supply an origin. These actions do not authenticate an original plan, infer an
unrecorded custody state or promote a partial SQL export to complete evidence.

The collector's retained reason21 transition from collector owner10 to system
owner7 with ID0/context0 represents quarantine. The collector command requires
state3 for that endpoint, and generic item transfers reject that reason. All
four SQL ownership projections therefore retain `quarantined` for this exact
contract, with action `move`. Destruction endpoints remain tombstones. Other
system custody, wrong source types and invalid system identities do not acquire
a quarantine inference. The ledger has no general custody-state column; this
specific producer rule does not supply missing state proof for other historical
transitions or certify a complete export.

Retained UID references, lineage history, unreferenced events and unattributed
history all accept this quarantined state. Unknown states still refuse or retain
the existing orphan-reference finding. Quarantined custody occupies a UID just
as live custody does: a previously retired UID returning in either state retains
the `resurrected_item_uid` finding.
See [the retained collector quarantine qualification](PLAN5_COLLECTOR_QUARANTINE_QUALIFICATION_2026-10-06.md)
for the native factory, both-engine history and preserved failure evidence.

All five detail projections require the exact decoded native representation,
including nested coin and owner vectors. An integer-valued JSON float or a
Boolean does not authenticate an integer field even when Python equality would
consider them equal. UID, root, non-null parent and destination-owner members,
posting scalar values, indices and revisions must be actual JSON integers equal
to their bounded native originals. A null parent must remain null. Representation
mismatches retain the corresponding `original_plan_*_mismatch` finding and do
not increase `checked.original_plans_verified`.

Each EAP1 is at most 4 MiB; their decoded input total is at most 32 MiB. The
whole JSON file remains limited to 32 MiB, including hex expansion. Before
fetching plans, the SQL exporter checks selected nonbaseline root count, total
hex-expanded plan size, and maximum individual plan size within the same
read-only consistent transaction. Exported EAP1 contains fixed-width numeric
facts and non-personal IDs. Opaque EAI1 facts are not exported. Operator views
omit canonical bytes and personal aliases; limits 0/1/100 preserve the global
audit result. `checked.original_plans_verified` counts roots whose plan and
captured projections passed these checks, including when no exception details
are requested.

This authenticates the captured projections against the supplied original plan
and persisted digests. It does not authenticate opaque original intent facts,
the complete command binding or receipt payload, attested native completeness,
or uncollected historical roots. A model-authored plan remains synthetic
evidence. SQL cuts remain `complete: false`; these checks do not qualify a
writer, authorize correction/activation, or satisfy the release matrix.

Native money and item revisions are exact JSON integers in 0..UINT64_MAX,
matching their unsigned native schema and wire fields. Boolean, negative or
larger values are malformed evidence. This differs from signed denomination
and copper-total ranges. Item events advance one revision; money effects retain
their native before/after revision chain. Origins, current authority and retained
creation/retirement roots must agree across the complete unsigned range.

Opening and current native item positions also require exact JSON integers
before UID indexing. `uid` is in 1..UINT64_MAX, `root` is in 0..UINT64_MAX,
and the required `parent` is either null or in 1..UINT64_MAX. `owner` is a
three-element list: its native owner type is an integer in 0..12, and its
identity/context are integers in 0..UINT64_MAX. The zero/unknown owner and root
remain available to the existing absent-creation and semantic checks; parsing
does not grant them live custody. Floats and Booleans never authenticate these
integer positions or UID keys. A malformed or missing position refuses every
CLI view with status 2 and the fixed `invalid item position` diagnostic, even
at limit zero, without rewriting the supplied snapshot.

The SQL opening-origin reader also contains EAB2 position and forest decoder
refusals within its `OriginError` boundary, using the fixed
`EAB1 committed root mismatch` diagnostic used by original baseline qualification.
A malformed selected book refuses
capture and still rolls back and closes the cursor. A malformed retained-epoch
book cannot authenticate a baseline claim; the existing claim consumer leaves
its witness unbound for reconciliation. Neither path repairs or reseals the
supplied evidence, and this reader behavior does not qualify native EAB2/schema61.

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

Persisted guild treasuries are likewise separate raw candidates:
`native.guild_treasuries` contains only `guild_id` and four unsigned-INT
denominations in `balance`. Every `guilds` row is included, including zero
and unloaded guilds. IDs are reusable native locators, not accounting lifetimes.
`outcome_revision` tracks prestige/construction and is not a money revision.
`native.guild_treasury_coverage` records rows, positive/zero rows and
missing-revision rows. The independent reconciler validates unsigned bounds,
unique IDs and exact collection shape, recomputes coverage and reports
`unsupported_native_guild_treasury` and `missing_guild_money_revision` for
each row. Missing SQL coverage reports `missing_guild_treasury_coverage`;
counts remain intact with detail limit zero. The exporter requires `guilds`
to be InnoDB within the same SELECT-only consistent cut. These raw values
remain outside mapped holdings: enrollment, durable lifetimes, origins,
monetary revisions and gameplay writers still need qualification.

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
  shop buy/sell, collector purchase and auction bid roots across the selected
  lineage, including outcome and the
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
  child operation ID and parent index, plus the original persisted `domain_id`
  (nonzero uint32), `discriminator` (uint64), `relationship` (integer 1), and
  explicit nullable `receipt_operation_id`. The independent reader derives the
  child ID as the first 16 bytes of SHA-256 over the original parent ID followed
  by the domain and discriminator in little-endian order. A nonzero parent
  index names an earlier child of the same root. Child IDs must be canonical,
  nonzero, distinct from their root and unique throughout the captured rows.
  A supplied receipt ID must equal its child. Missing derivation/receipt fields
  produce `missing_child_identity_evidence`; malformed facts produce
  `invalid_child_link`; a derived disagreement produces
  `child_identity_mismatch`; reused IDs produce `duplicate_child_operation`.
  Older child projections are unverified, never silently completed from the
  ID. These checks do not authenticate a child receipt's command or status.
- `item_references`: root/event index, UID, after revision, exact legacy
  ownership operation/event index, before and after revisions, and child
  index. `ownership_events`: the
  immutable UID history with its resulting owner/root/parent/state and action.
  `source_claims`: lineage, 48-byte source event and owning root ID. `receipts`:
  retained root ID, committed status and result code.

For each root, retained effect, posting and item-reference indexes cover exactly
`0..count-1`; child indexes cover `1..count`. Export row order may vary. When
the row count matches but these positions are sparse or offset, reconciliation
reports `evidence_index_mismatch` with the root ID and table. Cardinality and
duplicate findings retain their existing meanings. Ownership-event and legacy
reference indexes keep their original positions in the ownership ledger.

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

`scripts/economic_sql_audit_origins.py` reads retained EAB1/EAB2 baseline witnesses
through a dedicated repeatable-read, consistent-snapshot, read-only SQL
transaction. Give it an explicit host, database, user, lineage, epoch and new
output path; its password comes from the named environment variable (default
`DB_PASSWORD`). Use a database account with `SELECT` privilege only. It checks
InnoDB source tables, the control revision and terminal witness, each committed
baseline root/inbox receipt, stored SHA-256 and EAB1 framing, canonical row
order, nonzero source digests and unique account lifetimes/UIDs. A missing or
zero-revision baseline refuses. The output is bounded by the audit input limit
and contains only non-personal account keys, UID positions and revisions.

For schemas retaining `economic_baseline_witness.command_accepted_at_usec`, a
present admission time must be an exact positive unsigned64-bit integer. The
reader independently reconstructs the full original schema2 CCM1 command,
including that time and its original EAI1 intent, and compares its SHA-256 with
`critical_operation_inbox.command_hash`. Restore qualification consumes the
same independent comparison. The normalized EAI1 binding is checked separately.
Historical NULL times and older schemas without the column remain unknown;
neither reader infers a timestamp or authenticates a missing command preimage.
Their existing partial audit output cannot establish complete receipt or
release qualification. A known-time/hash disagreement refuses before export
and still rolls back and closes the cursor.

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
all captured rows agree. Before either coin-payload projection is fetched, the
exporter checks the stored coin collection's row count,32 MiB aggregate byte
limit and4 MiB individual payload limit. It also checks the mapping join's
aggregate bytes, counting repeated projections. Both projections select `NULL`
for non-coin payloads. These source limits supplement the encoded snapshot
limit; an oversized source refuses before fetching its payloads.
The exporter omits baseline root effects because the
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
the sum of its original lots for that account. Posting summaries are retained
across all 64-pair query batches. Original positive lot amounts stay immutable.

At canonical 0062, `native.pending_claim_consumptions` retains exact
`spending_operation_id`, `source_operation_id`, `source_slot` and positive
`amount` fields. The paired `pending_claim_consumption_coverage` contains the
exact integer row count. IDs are nonzero 16-byte lowercase hex, slots are
integers in1..65535, and amounts are integers in1..2^64-1; boolean, floating
point and string aliases refuse. Each collection is bounded to100,000 rows.
Historical SQL snapshots without these fields report
`missing_pending_claim_consumption_coverage`; absence is not an empty book.

The remaining amount is the immutable source amount minus its partial
consumptions, or zero for a legacy whole-consumption link. Remaining source
totals must equal the current native claim balance. A positive remainder
requires the original active PID; a fully consumed lot may retain a retired
mapping, whose original mapping/PID identity must still match. Successful
pending-claim debit roots are checked across reasons and epochs. The reconciler
refuses duplicate, missing, orphaned, mixed whole/partial, overdrawn and wrong
account allocations, and requires the allocated amount to match each debit
account as well as the root total. A consumption with neither retained source
nor spending root has unknown lineage; it remains a database-wide orphan
finding with its original IDs.

Selected-epoch consumer effects must also match the snapshot's independently
decoded original EAP1 plan. Auxiliary roots outside that epoch retain SQL
metadata/projection coverage; their original capsules require the separate
canonical SQL check under the release's quiescence procedure. Completeness
across legacy writers, original opening-policy/PID authentication and logical
source-event attribution remain unproven. The native allocation fixture is a
modeled history, and the 67-source batch probe tests SQL metadata only; copied
capsules do not authenticate its new root IDs. Linked
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

## Captured item position validity

Matching opening and current rows can still describe an impossible native
position. The independent reader checks both against the native custody
grammar and reports `invalid_item_origin_position` or
`invalid_native_item_position` with the UID. Unknown states, invalid owner
identities, destruction ownership on a live item, player ownership on a
tombstone, impossible root/parent fields and invalid equipment constraints do
not become an all-clear merely because both projections agree. Counts and
the whole-snapshot CLI status retain these findings at detail limit0.

Live and quarantined custody are both valid native states. A tombstone retains
destruction ownership and may preserve its former root or parent; those fields
are retained evidence. A logical creation opening represents its original
all-zero absent witness while naming the new UID. Missing historical equipment
remains unknown and is handled by the separate equipment audit. These checks
use the independent retained-evidence interpreter, without importing mutation
code or changing native rows. See
[the exact position qualification](PLAN5_ITEM_CUSTODY_POSITION_QUALIFICATION_2026-10-06.md).

The containment audit applies to both live and quarantined children. Parent and
child owner identity and root must agree throughout the captured ancestor path;
a quarantined child cannot use a destroyed parent as a current containment edge.
`inconsistent_native_topology` remains a finding even when opening and current
projections match. Valid forests may mix live and quarantined nodes. Detail
limit0 preserves the full finding counts and CLI status. See
[the exact quarantined containment qualification](PLAN5_QUARANTINED_TOPOLOGY_QUALIFICATION_2026-10-06.md).

## Quarantined coin diagnostics

The SQL exporter retains quarantined coin rows and their available denomination
payloads. The reconciler reads those rows and reports `quarantined_coin_pile`
with the UID; quarantine cannot make the entire diagnostic cut unreadable.
Quarantined coins remain outside active holdings and live-pile census totals.
Existing missing-holding, dangling-mapping, stale-custody and partial-capture
findings remain. Missing payloads are retained as unknown, and malformed states
or negative denominations still refuse. See
[the exact quarantined coin qualification](PLAN5_QUARANTINED_COIN_QUALIFICATION_2026-10-06.md).

## Retained flatfile intent and plan semantics

The independent flatfile restore reader validates retained generic EAI1/EAP1
semantics as well as physical checksums and command/intent bindings. A correctly
checksummed record still refuses when its policy, source kind, account effects,
postings, native item positions, containment forest or event replay is invalid.
The same refusal applies to the economic operator audit, restore preflight and
post-replay qualification. Readability remains subject to the separate opening
and lifecycle provenance gates.

Money checks preserve individual denomination legs, zero-sum copper, allowed
account kinds and system signs, increasing changed-state revisions, and exact
reconstruction. Item checks preserve equipment, valid quarantined custody,
destruction's retained former edges and all-zero absent creation witnesses.
The original flatfile child-reservation refusal remains. These checks use an
independent interpretation of the versioned wire contract; production mutation
code supplies qualification oracles only in tests. Every observation leaves
retained economic bytes unchanged. See
[the exact semantic qualification](PLAN5_FLATFILE_PLAN_SEMANTICS_QUALIFICATION_2026-10-06.md).

## Native mobile custody grammar

Original native-mobile custody uses owner type12 with a durable lifetime ID
from1 through `UINT64_MAX-1` and context0. The independent EAP1 and EAB1 readers
preserve that identity without inferring a runtime NPC ID or VNUM. Mobile live
and quarantined custody remain distinct; a destroyed item has destruction
ownership rather than native-mobile ownership.

An equipped mobile item in EAP1 must be active, have no parent, be its own root
UID, and use slot1 through43. Historical player equipment rules remain unchanged.
EAB1 has no equipment field; its owner12 identity acceptance does not prove a
complete equipped opening. Original EAP1 and the retained native boundary still
provide the required equipment and authority evidence. Native decoder agreement
and private SQL cuts remain component evidence, not quest producer completion.

## Original captured-item opening bindings

The SQL origin exporter accepts the optional `--captured-opening-evidence PATH`
input for one retained opening in the selected lineage and epoch. Its normal
origin export remains unchanged when this input is absent. The input is private
original evidence, with format `economic_sql_captured_opening_v1` and fields
`operation_id`, `legacy_native_boundary_digest`, `holding_coverage_digest`, and
`source_snapshot`. Identity and digest fields use canonical lowercase hex;
the operation is16 bytes and each digest is32 bytes. SQL NULL cells are JSON
null; every present raw SQL cell is lowercase hex, including empty strings and
embedded NUL bytes. Names, amounts, UIDs and payloads belong in protected
evidence rather than public diagnostics.

`source_snapshot` preserves the original native source DTO: version, exact
`rows`, `cells` and `cell_bytes` counts, all20 main tables, all3 physical item
source tables, and the separate version2 equipment table. It includes each
table's name, columns, definition/content digests and ordered rows with raw
cells and their digests, plus the ESM1, EIM1, EIE2 and ESC2 digests. The reader
independently recomputes ESD1, ESR1, EST1 and all manifest framing, including
the original distinction between NULL and empty bytes. It does not import
producer or mutation logic. Historical version1 requires empty equipment
and zero EIE2/ESC2 digests. A nonempty captured-item opening requires observed
version2 equipment.

The protected input must be a regular file with one link. POSIX permissions
must exclude group and other access; symlinks are refused where O_NOFOLLOW is
available. Duplicate JSON fields, malformed JSON and input above32 MiB refuse.
Native DTO ceilings also apply:262,144 rows,4,194,304 cells,64 MiB of raw cell
bytes and1 MiB per cell. The encoded operator-input budget can be reached
before the raw DTO ceiling. No input is truncated or reconstructed.

For each selected witness item, the reader resolves its original native row,
observed equipment row and matching owner-revision row. It preserves UID order,
exact ownership and containment fields, item revision/state and observed slot0.
It then checks EBS2, ESN5 and EIC2 against the retained canonical witness and
authenticated root. These new tags are raw bytes, and `frame(x)` is an unsigned
64-bit little-endian length followed by the exact bytes of x:

- EBS2: tag plus framed32-byte native, equipment and resolved owner-revision
  ESR1 row digests, in that order.
- ESN5: tag plus framed legacy native boundary digest, native item content
  digest, owner-revision content digest, equipment content digest and EIM1.
- EIC2: tag plus framed original holding coverage digest, unsigned64-bit item
  count, then each unsigned64-bit UID and framed EBS2 digest in witness order.

The equipment input to ESN5 is the table content digest, not the EIE2 manifest.
An empty witness preserves its original legacy boundary and holding coverage.
Altered, missing or incompatible original inputs refuse. The optional report
is `captured_item_bindings`, format `economic_sql_captured_item_bindings_v1`.
It records verified framing/bindings, witness and captured native item counts,
and whether the witness observes equipment.

This is a binding check with explicit authority limits. The supplied legacy
boundary and holding coverage digests still need independent authentication.
The original capture's provenance, complete item selection (including wallet
root exclusions), observed owner-revision consistency and complete source
coverage still need their own evidence. The report always leaves
`complete_item_selection_authenticated`, `legacy_digest_authority_authenticated`,
`original_capture_provenance_authenticated`, `complete_source_capture_authenticated`,
`activation_qualified` and `release_qualified` false. A matching empty witness
with a nonempty capture cannot authorize exclusions. Retained original evidence
can match while a later native capture differs; neither observation authorizes
activation or corrects data. All operator SQL reads remain SELECT-only in the
existing repeatable-read transaction, ending with rollback and cursor close.
See [the exact binding qualification](PLAN5_CAPTURED_ITEM_BINDING_QUALIFICATION_2026-10-06.md).

## Qualification budgets

The isolated native flatfile restore qualifier validates every retained
`domains/quest-mobile-native-<id>.qmn` image after authority-bundle recovery, in
both state preflight and final qualification. It requires a canonical nonzero
decimal lifetime ID below UINT64_MAX, a private regular file with one link,
the existing 4 MiB native image bound, canonical native reference/stock/cash
bytes, and exact filename/reference identity. Corrupt values or malformed names
in the protected namespace refuse with the existing fixed diagnostic. The image
check does not write those files or materialize a mobile.

Historical v1 images retain unknown cash; v2 images validate exact denominations
and cash revision. Retired images retain their original lifetime and have no
stock or cash. These are native value-format checks, separate from the
independent economic evidence reader. They do not authenticate birth/source
authority, resolve runtime custody or qualify complete opening, activation or a
release. The surrounding original restore and lineage checks remain required.

SQL restore and the standalone canonical audit independently interpret every
`quest_mobile_native.canonical_image`, with 256-row ID pagination and 64 KiB
capsule chunks. Each row's mobile identity, mobile/stock revisions and lifetime
state must exactly match its canonical QMNIMG v1/v2 reference and image. The
reader checks image/reference checksums, literal stock grammar, contiguous DFS,
equipment, shared nested-row/string/depth bounds, cash revision and denominations,
and retired stock/v2 cash. Historical v1 cash remains unobserved. The standalone
audit includes these rows in its 100,000-row collection limit and the images in
its 32 MiB aggregate capsule budget. Both consumers use SELECT only and refuse
corruption; neither authenticates birth/source admission, compares complete
live-world custody, creates money holdings or provides activation authority.

The maintained `test_native_sql_baseline_audit.py` recipe also exercises the
full independent SQL snapshot reader before and after actual dump/import into
new private MariaDB and MySQL daemons. Its EAB2 books and commands come from the
original native fixture; its two current equipment placements are explicit
modeled inputs. A slot-only discrepancy must remain visible after restore,
with identical exception totals at detail limits0,1 and100. SELECT-only roles,
transaction rollback, and all application-table data hashes are checked.
The retained-evidence qualifier and native replay can pass while the live-state
reader reports equipment drift; those observations have different scopes.
This recipe retains its partial-capture findings and cannot qualify a complete
world backup, managed backup generation, service boot or release. See
[the exact cold-restore qualification](PLAN5_EQUIPMENT_COLD_RESTORE_QUALIFICATION_2026-10-06.md).

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

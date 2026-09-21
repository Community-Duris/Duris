# Economy accounting contract

Status: **phased foundation; draft coverage, guarded wire support, no accounting
execution or release qualification**. Parent #474; partial delivery for #475 and #476. Source census
baseline: `48c0aedd8e094eee37285111e46e735e4cf12320`.
See [phased delivery and acceptance](economy_accounting/DELIVERY_PLAN.md).

The machine-readable contract lives in `economy_accounting/registry.json`,
`golden.json`, and `writers.json`. Run
`python3 scripts/validate_economy_accounting.py` to validate contract examples
and census drift. `--release` additionally requires frozen policy and qualified
coverage. Passing the draft validator establishes neither implementation nor
backend support. Unclassified candidates block release.

## Economic authority and commit

An economic operation combines coin postings and item custody events under one
root operation ID. These are independent conservation constraints: transferring
an item cannot balance a coin deficit, and balanced coins cannot authorize an
item move. Existing child operation IDs remain immutable and link to the root.

The critical-command coordinator remains the only admission, fencing, retry,
receipt, and publication mechanism. Accounting does not introduce a second
queue, writable item ownership authority, or mutation path. Admission records a
bounded intent. Under existing authority locks the worker resolves that intent
against authoritative state, validates the complete plan before changing state,
applies that plan, verifies exact effects, and commits domain state, immutable
evidence, and receipt atomically. A balanced but unauthorized plan must fail.

SQL evidence uses the same database transaction as its domain changes. Every
successful commit branch must include effect verification, including compound
commands and existing nested repository paths. A savepoint rollback also rolls
back accumulated plan effects. Flatfile evidence and indexes are after-images
in the same checksummed authority transaction as domain state and receipts;
recovery completes before a subsequent mutation is admitted.

Live object publication, save snapshots, telemetry, outbox delivery, and reward
projections follow economic commit. They must not create additional economic
postings. Failed publication retains the committed receipt and recovery work.
An ambiguous receipt is unresolved, never permission to issue a replacement ID.

Exact ID and identical canonical request bytes return the original receipt and
evidence without recompiling against today's balances or policy. Same ID and
changed bytes fail. Policy version, intent digest, resolved-plan digest, lineage,
epoch, and source-event identity are retained. Old receipt dedupe survives
checkpointing, retention, epoch changes, backup, and restore.

## Coin identities and arithmetic

Values are signed integer copper; denominations are copper/silver/gold/platinum
with multipliers 1/10/100/1000. Every leg retains four signed denomination deltas
and their checked copper value. Ordinary holdings are nonnegative in each
denomination and in value. Compute using checked wider intermediates, reject
out-of-range stored values, and require the exact per-operation copper sum to
be zero. Denomination deltas describe actual before/after holdings, including
making change; they are not merely a signed decomposition of the scalar price.

A durable account key is explicitly encoded, independent of ABI padding. Its
40-byte budget contains world lineage, account kind, authority ID, and typed
context with version/reserved fields. Lineage identifies the durable world
history and survives restore; it is distinct from a baseline epoch.

| Account | Identity and boundary |
| --- | --- |
| Wallet | Durable, non-reused player incarnation mapping; never display name or pointer. |
| Shared bank | Permanent bank mapping plus racewar scope. SQL uses the locked native bank identity and database collation. Flatfile requires an equally durable mapping; a lowercase name hash is insufficient. |
| Coin pile | Item UID, with the item lifetime retained after destruction. A custody move does not itself change pile value. |
| Auction escrow | Durable auction incarnation, funded when a bid is accepted. |
| Pending claim | Stable claim holding with typed origin and original source-operation attribution. Existing aggregate claim storage may be retained with explicit operation links; new per-claim rows are not required. Includes auction refunds/proceeds and existing non-auction reward/compensation pickups. Collection transfers from this holding. |
| Treasury | Existing real durable holding only. Do not invent finite shop/NPC money. |
| Issuance | Named, typed policy, debit-only, with a durable source event. |
| Sink | Named expense/destruction policy, credit-only except an explicitly authorized reversal tied to an original committed expense. |
| Opening equity | Exact baseline/epoch reconciliation only; cannot fund gameplay. |
| Restitution | Explicitly authorized new issuance, distinct from a transfer correction. |

Mappings retain backend locator, creation operation, retirement operation,
revision, and mutable aliases. Rename preserves identity. Deletion and later
recreation allocate a new lifetime. Native IDs that can be recycled require a
retained mapping. Account or reason strings supplied by callers do not confer
system-account capabilities. Versioned writer policy constrains permitted
accounts, signs, source event, actor, and exact authoritative effects.

The draft golden `opening` case represents accounting opening positions only;
it does **not** authorize adding money to an already populated gameplay wallet.
Baseline reads a quiescent consistent snapshot and introduces the observed
balances into accounting against opening equity without mutating assets.

## Resolved plan encoding

The pure `economic_accounting_plan` codec uses `EAP1`, a 256-byte header,
fixed-width little-endian fields, explicit version 1, and zero reserved bytes.
The header binds lineage, epoch, root/original operation IDs, actor and writer,
policy/compiler versions, reason, optional 48-byte source identity, and separate
32-byte intent and domain digests. The source identity contains a numeric kind,
source and generation IDs, sequence, and slot; its presence alone grants no
issuance or replay entitlement. Numeric source kinds are in `registry.json`.

Rows retain complete account effects (120 bytes), ordered coin postings (48),
child links (32), item snapshots (64), and item events (128). Item positions use
56 bytes, including eight trailing reserved bytes. Accounts sort by semantic
identity; snapshots sort by UID; postings and events sort by stable event index.
Child links use a deterministic topological order, choosing the smallest
available operation ID, with every reference remapped. No audit leg is netted
away. Decoders reject noncanonical wire ordering instead of silently rewriting
it. SHA-256 covers the entire canonical encoding. Exact size and all count caps
are checked before allocating decoded collections. Failure leaves outputs intact.

An ordinary account whose denominations do not change may appear without a
posting only when its authoritative revision advances. Existing currency writes
advance both wallet and bank revisions, including a balance that is unchanged.
This effect remains in the plan without inventing a zero-value ledger leg.
Typed adapters must still prove that every such revision change really occurs.

The codec checks structural policy (account kinds, actor class, required source
and original operation, system-account signs), conservation, topology, and
before/after agreement. It does not grant writer capabilities or verify a source
receipt. Locked-authority adapters, frozen intent, coordinator integration,
atomic storage, and source deduplication remain required before enforcement.

### Immutable command binding

`economic_command_binding_digest` hashes the NUL-terminated domain tag
`DURIS-ECONOMIC-COMMAND-V1` followed by canonical schema-1 critical-command
bytes, with only the projection's acceptance timestamp set to 1. It normalizes
key/revision ordering without changing the supplied command, rejects duplicate
keys and invalid/over-limit inputs, and binds operation ID, command/payload
versions, source/deadline, keys, revision conditions, and exact domain payload.
This digest remains stable when admission assigns or restores acceptance time.
The existing full command hash, journal bytes, and receipt equality retain the
actual timestamp. The current helper accepts schema 1 only; future envelope
support must explicitly preserve this projection while excluding the accounting
extension from this preimage to avoid circular digest dependencies.

## Item custody and admission

Reuse `item_current_owner` and `item_ownership_ledger`; add immutable root/child
and event links plus before/after topology to that history. No competing mutable
ownership catalog is permitted. Every event has a consecutive index in its root
operation and an existing linked operation ID. One UID may have multiple ordered
events in a compound operation. Each before-state must match the preceding
state; final custody must be unique, acyclic, rooted, and owner-consistent.
Same-owner container or equipment changes are events, not no-ops.

Creation or documented first admission accounts for an asset exactly once.
Template allocation alone does not prove creation: it also serves restoration,
inspection, temporary comparison objects, and failed staging. Loading saved
items, world materialization, copyover, pet hydration, or rebuilding a projection
is not issuance. Extraction likewise serves real destruction, unload, staging
rollback, and cleanup; instrumenting every `extract_obj` as destruction is wrong.

Durable source events distinguish a logical issuance from a new retry ID.
Their proposed 48-byte encoding contains kind/version, durable source ID,
lifecycle generation, sequence, and slot. Writer-specific policy defines which
fields establish uniqueness and retains dedupe through lifecycle transitions.
Process-local NPC runtime IDs and prototype VNUMs are not durable lifetimes.
Transient NPC exceptions and unsupported endpoints remain explicit refusals
until a reviewed authority exists; do not fabricate durable NPC custody.

The covered supply boundary includes admitted wallets, shared banks, durable
money piles, escrow/claims, and actual durable treasuries, plus admitted item
UIDs. Legacy unknown origins are reported as unknown/opening evidence, never
quietly relabeled issuance or valid history. Destruction of a money pile includes
its coin sink and item event in one operation. Moving that pile changes custody
without minting or sinking its contents.

Saved-item recovery depends on merged #469/#495: preserve source epoch/root,
exact payload digest, durable handoff receipt, acknowledgement, and retirement
ordering. Restoration consumes that identity rather than minting it. Pet custody
uses durable pet UID and master PID, not a process ID or a recycled row ID.

## Bounded intent and realized plan

Preserve the current command envelope and domain payload capacity. A common
versioned extension has a four-byte length followed by at most 8,192 bytes:

`52 + 40 * 3003 + 393216 + 4 + 8192 = 521584 < 524288`.

The intent references already hashed domain payload or revision-guarded domain
state through typed versioned selectors, never arbitrary SQL, text, or pointers.
It contains lineage/epoch/root/parent identity, actor/source identity, reason,
writer/compiler/policy versions, payload/evidence digests, and checked counts.
Tentative record budgets are 40 bytes/account, 48 bytes/posting, 32 bytes/child,
and 16 bytes/item range, with a 256-byte header. Individual maxima are 64
explicit accounts, 64 explicit postings, 64 child links, and 32 item ranges.
These maxima cannot all be filled together: the aggregate 8,192-byte cap also
applies. Reserve bytes must be zero and unknown versions fail explicitly.

Under locks, the realized plan may contain at most 3,072 accounts, 6,144 coin
legs, 3,000 item events, 64 child links, and 4 MiB total. Item validation may include up to 6,000 affected/ancestor
witnesses in each before/after snapshot; adapters must read the complete affected
forest under authority locks. Absent entries and destruction tombstones remain
explicit, and ordinary creation cannot reuse a destruction tombstone. A tree of 3,000 money
piles can require 6,000 legs for admission/destruction, so the explicit-intent
64-leg cap cannot be reused as a realized-plan cap. Pure custody movement of
the same tree requires no coin legs. Evidence pages are capped at 256 rows;
recovery batches at 32 operations. These are safety ceilings, not measured
performance guarantees; #490 must set and measure workload budgets.

Keep existing admission limits (1,024 pending operations, 64 MiB, two workers)
and authority transaction ceilings. Bounded append/index/checkpoint work must
replace existing whole-catalog rewrites and finite receipt catalogs where needed.
Adding a separate bounded journal while an existing receipt store still reaches
ENOSPC does not satisfy bounded durable operation. Checkpoints never forget
operation/source-event dedupe. Capacity errors refuse before economic mutation.

Existing first admission followed by movement uses separate durable operation
IDs, each with its own event limit (`item_movement_transaction.c`). Batch
movement requires already adopted roots unless it is creation. Combining these
stages into one atomic operation later requires an explicit event-cap review.
Destroyed child rows retain their historical root/parent topology in existing
authority; accounting must preserve those fields rather than normalize them to
standalone roots. Tombstone edges are not current live containment edges.

## Examples and validation

Golden cases cover wallet/bank deposit and withdrawal; making change into a
pile; a three-party split; issuance; expense; same-owner container movement;
a 100-gold sale paying the seller 95 gold and fee sink 5 gold; staged auction
listing, bid, settlement and collection; outbid refund; opening equity; and
money-pile admission/destruction; distinct shared-bank contexts; and nested
item destruction retaining former topology. Staged settlement debits escrow, not the buyer
wallet again. Item collection remains a separate custody event with linkage.

The Python validator is an executable contract model, not a storage simulator.
Negative holdings, overflow, unauthorized counterparties/signs, unbalanced sums,
invalid identities, changed replay payloads, repeated source events, stale item
before-states, and invalid final topology fail. Independent production
reconcilers must compare actual holdings/custody to immutable committed evidence
without trusting the same mutation code. Projection compatibility and legacy
coverage are reported separately.

## Writer inventory and qualification

Each reviewed writer records path/symbol/call sites, backend, authority boundary,
source/sink classification, reason, current support, integration owner, and
executable test candidates. A test candidate is not a passing test. The lexical
census deliberately includes allocation/publication/cleanup sites for review;
it is neither a complete semantic census nor evidence that a hit mutates money.
Direct fields, SQL, administrative and lifecycle paths also require review.

Coverage states are legacy, observed, enforced, unsupported, and projection.
Observed evidence does not gate the writer. Enforced evidence participates in
its authoritative commit. Unsupported requires the actual refusal plus an
executable regression; projection requires proof it cannot write authority.
Release requires all relevant backends, no unclassified sites, independent
reconciliation, old-receipt replay, and lifecycle registration.

| Slice | Deliverable |
| --- | --- |
| #475 | Frozen contract, complete classified census, fixtures/validator. |
| #476 | Bounded pure plan API, codec and coordinator integration. |
| #477 | Atomic SQL storage, additive migration and registration. |
| #478 | Atomic flatfile evidence, indexes, checkpoint/recovery. |
| #479 | Consistent baseline and explicit activation epochs. |
| #480 | Generic currency transfers and holding adapters. |
| #481 | Typed issuance, expenses, source dedupe. |
| #482 | Item history and root/child/event linkage. |
| #483 | SQL and flatfile shop paths, including SQL legacy behavior. |
| #484 | Collector integration. |
| #485 | Auction escrow, claims, fees and custody. |
| #486 | Death, world creation/recovery, destruction and restoration. |
| #487 | Independent reconciliation, audit, compatible projections. |
| #488 | Guarded immutable corrections and restitution. |
| #489 | Delete/reset/export/erasure/retention/backup/restore. |
| #490 | Fault tests, measured budgets, rollout/runbooks and qualification. |

Deliver linked, dependency-ordered PRs for `xander-l` review. Merging a foundation
does not activate accounting or complete its parent issue. Preserve existing
separately owned fixes and verify their current interfaces rather than duplicating
them. PR #409 has merged on this baseline; kingdom/workshop routes belong in the
writer inventory. These foundation increments add no storage or gameplay callers. Frozen intent
and schema-2 wire support are guarded by legacy-only execution checks; see
[economic intent design](economy_accounting/INTENT_DESIGN.md).

Migrations remain additive and sealed history remains unchanged. New tables,
flatfiles, codecs, manifests, exports, erasure, reset, backups and rollback
contracts are registered together. Corrections append linked evidence against
verified expected state and original evidence; they never edit history or hide
imbalances with arbitrary adjustments. Financial retention and erasure use
retained non-personal identities with governed alias removal.

Activation proceeds through legacy/observation/qualified enforcement with
explicit compatibility and pause gates. After activation rollback must pause
writers or retain an accounting-capable binary and all history/dedupe; no silent
unjournaled fallback is allowed. Local MySQL, MariaDB and flatfile builds,
focused regressions, actual synthetic player journeys, fault/restart/replay,
ASan/UBSan and the full applicable test suite are required. Hosted CI does not
replace these checks. Production migration, deployment, wipes, or restitution
are outside this implementation authorization.

## Census decisions still preventing contract freeze

- Floor coin drops currently commit a wallet debit before pile publication;
  NPC gifts cross a separate live credit. Group split credits recipients before
  the sender debit. These need explicit compound-operation integration.
- Failed `ADD_MONEY` admission can create a SQL pending pickup outside the
  command receipt protocol. The pending-claim identity therefore covers
  non-auction rewards and compensation as well as refunds/proceeds.
- Blackjack has real live stakes on the table. A push returns the original
  stake; a win distinguishes returned stake from net issuance. Durable holding
  and round identity are still unresolved.
- Roaming shopkeepers have finite cash checks and postcommit live cash changes.
  Neither a generic treasury label nor infinite NPC issuance proves authority.
- Trusted auction removal currently permits a winning bid without staging a
  bidder refund on either backend. Account for the existing disposition
  explicitly; do not silently introduce a reimbursement or change policy.
- Collector purchases have no seller payout and use a named purchase sink.
  Disabled shop gem barter must remain disabled.
- Crafting, smithing, and refinement consume inputs separately from durable
  output grants. A creation receipt alone does not prove an atomic recipe.
- General archive/export/erasure controllers remain disabled by their canonical
  policy, whereas character/account deletion is executable. Preserve these
  refusals and register accounting data without inventing policy approval.

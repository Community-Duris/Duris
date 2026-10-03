# Economy accounting phased delivery

## Current delivery tracking (2026-10-03)

Use [Finish accounting, saves, item custody, and death recovery](FINISH_ACCOUNTING_PLAN.md)
for the execution order on `experimental-accounting`, and
[#490](https://github.com/Community-Duris/Duris/issues/490) as the single remaining
implementation and qualification tracker. Outstanding reconciliation (#487),
corrections (#488), lifecycle/restore (#489), initialization (#568) and sourced
NPC vial issuance (#661) are consolidated into its numbered acceptance sections.
Their closures are supersession records, not completion or scope-removal claims.

[#664](https://github.com/Community-Duris/Duris/issues/664) remains separately
open for historical quarantined-case reconciliation. Its prevention and stopped
recovery tooling are delivered; private case eligibility and authorized resolution
remain distinct from technical feature completion and production reopening.
[#551](https://github.com/Community-Duris/Duris/issues/551) is completed through
PR #693; preserve its scoped active poison/Encrust/Harvester evidence.

The five domain plans and R1-R8 contract remain required. Historical phase IDs
in writer metadata retain their requirement identity without implying qualified
coverage. Link focused implementation PRs to the relevant #490 section, retain
file-owner handoffs, and record backend proof and tested revisions in the existing
evidence documents. SQL is the first milestone; MySQL, MariaDB and flatfile remain
required for full completion. Production operations retain separate authorization.
Older commit-specific status statements below are historical; verify them against
current code before acting.

## Prior plan baseline: add-double-entry at 49af585c4 (2026-09-27)

The [remaining requirements](REMAINING_REQUIREMENTS.md) and five work plans
below supersede the phase order later in this file. SQL is the first delivery
target; flatfile is required before the full feature is complete. Item economy
means durable UID provenance/custody/supply plus actual trade prices. Appraised
inventory values are outside this feature.

| Independently executable plan | Owned result | SQL milestone | Full-feature milestone |
| --- | --- | --- | --- |
| [1. Authority and cutover](plans/01_AUTHORITY_AND_CUTOVER.md) | Coordinator dispatch/reconcile, atomic owner, staged baseline, publication, guarded activation | Coin and item schema-2 commands traverse the real pool and replay correctly; an incomplete census cannot activate | Equivalent flatfile authority and replay |
| [2. Money and supply](plans/02_MONEY_AND_SUPPLY.md) | Ordinary holdings, transfers, coin piles, rewards, expenses, source dedupe | Every supported general money writer has balanced native/evidence/receipt proof | Same policy and journeys on flatfile |
| [3. Item provenance](plans/03_ITEM_PROVENANCE.md) | UID admission, custody/topology, intentional retirement, restoration identity | Every supported ordinary item event has one exact reference and valid source | Same UID and source behavior on flatfile |
| [4. Compound domains](plans/04_COMPOUND_DOMAINS.md) | Shop, collector, auction, craft, death/resurrection composites | Domain-specific player journeys prove money and item effects in each commit | Equivalent domain journeys and refusals on flatfile |
| [5. Audit and release](plans/05_AUDIT_OPERATIONS_AND_RELEASE.md) | Complete writer inventory, independent reconciliation, operations and fault proof | Zero unclassified real writers and SQL reconciliation/report | Both backends and release matrix pass |

The historical requirement crosswalk is Plan 1: #476-479; Plan 2: #480-481;
Plan 3: #482; Plan 4: #483-486; Plan 5: #475 and #487-490. Current acceptance
is owned by #490's numbered sections, as mapped in the active completion plan.
Historical issue closure does not satisfy baseline/activation or the joint
release gate; #490 closes only after its complete technical acceptance passes.

Each plan starts from this head and has its own inactive-epoch fixtures,
owned code areas, and acceptance evidence. Work can proceed concurrently
without selecting an active epoch. Shared coordinator changes are owned by
Plan 1; domain plans add narrow typed adapters and publish their interface
needs instead of introducing a second queue or authority. Integrate branches
against one tested head, then run the joint route inventory and gameplay
matrix. Independent execution does not imply independent activation.

The immediate SQL integration gate is concrete: the SQL direct root handles
bank, coin, and item schema-2 families, but pooled apply/reconcile still gate
on the bank family. The staged SQL baseline covers wallet, shared bank, and
active coin-pile balances and leaves active_epoch null. The production boot
guard is wired, yet no complete writer-set activation path exists. The draft
writer census currently fails its drift validator. The tracked route matrix
includes the new corpse item route, but still cites an older repository head
and does not count later SQL coin evidence; reconcile it with current code
before quoting any coverage count as release evidence.

### Unified release gate

1. Reconcile a quiescent complete source snapshot, active epoch, mappings,
   native state, and immutable openings without creating gameplay money/items.
2. For every reachable writer, prove typed same-root accounting on SQL and
   flatfile or an executable refusal before mutation; classify projections
   and dead code separately. No legacy schema-1 bypass is allowed in an active
   epoch.
3. Prove conservation and exact custody using an independent read-only
   reconciler, then real player journeys for transfer, issuance/expense,
   commerce, crafting, death/recovery, and restart.
4. Prove exact-ID and source-event dedupe, lost-reply reconciliation, rollback,
   publication, backup/restore, erasure/retention, and measured workload
   budgets. Keep a pause/rollback path that never permits unjournaled writes.

No plan completion authorizes production migration, deployment, wipes, or
restitution. Use disposable local databases for SQL acceptance. Preserve
existing branch changes and apply the smallest typed seam needed: the current
critical-command authority, current money and item ledgers, and current UID
catalog remain the sources of truth. No new inventory appraisal, second
custody catalog, or general online freeze framework is needed.

## Historical phase record (superseded as an execution order)

Approved direction: 2026-09-21. Parent [#474](https://github.com/Community-Duris/Duris/issues/474).
Linked phase PRs supersede the previous one-final-PR instruction. The full feature
contract and all 16 child issues remain in scope; no phase merge authorizes a live
cutover or production deployment. Request **xander-l** review on each PR.

## Historical 2026-09-26 priority: database backend first

Updated by user direction on 2026-09-26: prioritize the SQL backend (MariaDB/MySQL).
Flatfile catch-up is a separate, non-blocking workstream; it must not consume the
primary implementation or validation effort. Existing flatfile work is preserved,
not declared complete or removed. The historical phases and both-backend exit
criteria below describe the full-feature backlog, not prerequisites for the next
DB milestone.

The active DB order is:

1. Qualify the combined branch's SQL schema, transaction and custody contracts.
   Reuse still-valid passing evidence; retry only concrete failures after each
   coherent batch. Provision a disposable fixture for DB-required tests.
2. Finish durable death-conflict admission, authenticated player-visible recovery
   and unassisted terminal release, without inferring ownership from evidence,
   deleting conflicting payloads, weakening bindings or forcing extraction.
3. Complete SQL lifecycle/activation authority and real currency/item producers,
   including post-commit visibility, retry, reconnect and restart recovery.
4. Qualify corpse/raise-dead, item creation and item-flag spells against the DB
   backend, with current-run persistence/WIZLOG review and durable read-back.

Active validation uses the DB server build and focused SQL/disposable gameplay
checks. Full flatfile builds, native flatfile regression suites and cross-backend
parity are deferred and must be reported as deferred, never as passing. Small
shared-type/codec checks remain applicable when they protect the DB contract.
Accounting activation and death release still require their real acceptance
proofs; backend reprioritization does not waive those safeguards or authorize
production changes.

### Qualified inactive-SQL restoration

Legacy inbox IDs are not accounting admissions. The item/coin/auction/collector/
corpse/restitution paths no longer manufacture accounting contexts or implicitly
install an epoch. Their existing custody, binding, revision and transaction checks
remain in place. Explicit accounting contexts still fail closed on malformed IDs,
out-of-range lines, missing implementations or failed references.

These six legacy dispatch routes now hold the shared maintenance/writer fence
from before the transaction through commit, rollback or replay, and refuse both
staged phases and any active epoch. Missing lifecycle schema also refuses writes.
The real SQL regression first reproduced a grant committing during phase 1 before
the fence correction, then passed on MariaDB 10.11 and MySQL 8 with native-state
preservation, injected rollback, successful commit/replay and lock-release checks.
This is not source-complete writer authority or permission to activate accounting.

The DB build and fresh healthy death/loot/reconnect journey pass. The guarded
death-conflict journey still fails unassisted account-menu release: existing
payload/custody rows remain intact and the one added coin is funded by the wallet,
but durable terminal recovery and authenticated recovery visibility are not wired.
The dormant archive's SQL matrix, concurrent replay, lost-commit-reply recovery
and restart read-back pass on both engines; archive durability alone does not
authorize extraction or materializing disputed items.

### Deferred flatfile catch-up

Bounded catch-up was received and verified separately on
`work/flatfile-catchup-db-priority` at
`ee774be89114d7db5a4c0c9275737a0faf1446d3`. It remains parked, not integrated or
published with this DB checkpoint, and is not a DB completion gate. Follow-up scope:

- Validate the inherited auction, collector, shop, universal-item and item-reference
  tests with the configured compiler; retain sanitizer/warning checks. The prior
  merged batch failed to start these compiles because they required `g++-12`.
- Resolve known flatfile formatting drift and subsequent native compile/test
  failures without changing shared SQL contracts merely to make flatfile pass.
- Bring lifetime/lifecycle, cutover and source-complete currency/item writers into
  parity with the verified DB contract, preserving unsupported-operation refusal.
- Implement and qualify native durable conflict recovery, authenticated recovery
  visibility, publication acknowledgement, replay and restart behavior. SQL archive
  retention alone does not establish a flatfile recovery implementation.
- Recheck corpse/raise-dead, creation/flag spells and failure/recovery journeys;
  finish with a clean flatfile build, native fault tests and explicit parity evidence.

The worker's detailed `FLATFILE_FOLLOW_UP.md` is in that isolated branch. Later
integration, full native validation and overall cross-backend qualification remain
incomplete.

## Historical increment: read-only SQL source capture and normalization

Based on [PR #612](https://github.com/Community-Duris/Duris/pull/612) at `dab03b0e6`.
Reuse bounded native capture and typed normalization from `ecfee1218f` and
`f96949f7f`. Preserve exact selected bytes and source references, report native
contradictions, and leave source state unchanged. See
[SQL_SOURCE_SNAPSHOT.md](SQL_SOURCE_SNAPSHOT.md). This is selected evidence, not
complete inventory coverage or a cutover capability. Lifetimes, enrollment,
maintenance ownership, publication acknowledgement and activation remain pending.

## Prior increment: SQL baseline witness and reservation storage

Based on [PR #610](https://github.com/Community-Duris/Duris/pull/610) at `a039c6c94`.
Reuse the bounded schema and private transaction owner from `e52e18003` and
`2c2469cff`, adapting the unpublished migration to `0032`. Retain complete witnesses,
reserve each identity once per epoch, and reconcile exact-ID retries after an
ambiguous commit. See [SQL_BASELINE_STORAGE.md](SQL_BASELINE_STORAGE.md).
Native source capture, wallet/shared-bank enrollment, maintenance ownership,
publication acknowledgement and activation remain pending. This component does
not change gameplay coverage or close #479.

## Prior increment: flat-file baseline witness and reservation storage

Based on [PR #609](https://github.com/Community-Duris/Duris/pull/609) at `5fd726388`.
Reuse `ad840cf4c` private baseline storage, adapted to current v2 journal framing
and DURECR2 failure-stage validation. Retain complete witnesses and unique
per-epoch openings with lifecycle and backup registration. See
[BASELINE_STORAGE.md](BASELINE_STORAGE.md). Native source proof, lifecycle
admission, maintenance ownership and activation remain pending. The SQL counterpart
is the current increment.

## Prior increment: baseline preparation and retained source witnesses

Based on [PR #608](https://github.com/Community-Duris/Duris/pull/608) at `190602263`.
Reuse pure preparation, EAB1 witness encoding and EBC1 command binding from
`5c7d0683c`, `53fd01af9` and `dac52b03f`. Execution admission remains closed;
this component does not read or mutate native holdings, establish a cutover
boundary, persist openings or activate accounting. See
[BASELINE_PREPARATION.md](BASELINE_PREPARATION.md). Native baseline stores and
wallet/shared-bank enrollment follow, alongside the required maintenance and
publication recovery boundary before gameplay activation.

## Prior increment: flat-file bank dispatch and admission

Based on [PR #607](https://github.com/Community-Duris/Duris/pull/607) at `790665585`.
Pair the existing bank-only validator with the native flat-file transaction owner
at server startup. Preserve legacy dispatch and refuse unsupported schema-2 roots.
See [BANK_ADMISSION.md](BANK_ADMISSION.md). New held schema-2 bank commands now
restore their durable publication-retention flag on replay, with no checkpoint
before explicit acknowledgement. This coordinator qualification is separate from
the missing live gameplay publication/save and reconnect handoff: implement and
test that full path before activating wallet/bank producers.

## Prior increment: typed flat-file bank owner

Based on [PR #606](https://github.com/Community-Duris/Duris/pull/606).
Reuse borrowed-lock native reads from `42cacc40e` and the standalone bank owner
from `d47c7af0b`, adapted to DURECR2 failure-stage verification. Preserve 4096-byte
accounting results and the separate 2048-byte legacy receipt limit. See
[FLATFILE_BANK.md](FLATFILE_BANK.md). Backend dispatch/admission follows; gameplay,
baseline, lifecycle ownership and activation remain pending.

## Prior increment: retained flat-file authority metadata

Based on [PR #605](https://github.com/Community-Duris/Duris/pull/605), including its
boot-topology test fix. Reuse `6698326e4` lineage, epoch and lifetime metadata,
with retained epoch lookup and allocation-error preservation. Register all four
metadata file classes for lifecycle/backup. See [FLATFILE_AUTHORITY.md](FLATFILE_AUTHORITY.md).
Native lifecycle changes, baseline and gameplay activation remain pending.
The next typed bank increment needs the borrowed-lock native reads from `42cacc40e`
and the bank owner from `d47c7af0b`, adapted to DURECR2 failure-stage checks while
preserving the separate 2048-byte legacy receipt limit.

## Prior increment: bounded flat-file evidence storage

Based on [PR #604](https://github.com/Community-Duris/Duris/pull/604). Reuse
preserved bounded storage and its authority-journal bridge, including subsequent
durability fixes. Register evidence indexes/segments for lifecycle and backup.
See [FLATFILE_STORAGE.md](FLATFILE_STORAGE.md). Retained lifetime metadata and
the typed flat-file bank owner follow before backend admission or activation.

## Prior increment: typed SQL bank admission and replay

Based on [PR #603](https://github.com/Community-Duris/Duris/pull/603). Reuse the
bank-only coordinator extension from preserved `a89fa8f18`, pair it with the SQL
pool root, and retain default/flat-file refusal. See [BANK_ADMISSION.md](BANK_ADMISSION.md).
Gameplay, baseline and activation remain pending.

## Prior increment: typed SQL bank root

Based on [SQL storage PR #602](https://github.com/Community-Duris/Duris/pull/602)
plus independent [boon prerequisite #601](https://github.com/Community-Duris/Duris/pull/601).
Reuse exact prepared currency mutations, typed bank effects and the existing SQL
component; connect its direct root apply/replay checks while preserving current
failure-stage metadata. See [SQL_BANK.md](SQL_BANK.md). Pooled/coordinator and
flat-file schema-2 admission remain closed; no gameplay producer is enabled.

## Prior increment: SQL storage and retained identity locks

Based on [guarded-envelope PR #600](https://github.com/Community-Duris/Duris/pull/600)
at `511d04f16b613a000a857af4293fcd8b2d3fb48a`, including foundation contract
amendment `881663d68946bd80f72f720a2e5c368711023c3b`. Reuse the nine-table
schema and transaction-borrowing identity helper from the preserved implementation.
The provisional migration is 0031 because canonical 0030 is telemetry quarantine.
See [SQL_STORAGE.md](SQL_STORAGE.md) for scope, required evidence and the remaining
append/finalize, access-control and transaction qualification gates for #477.
No gameplay accounting is activated. Storage registration and native schema/identity qualification now pass; the
PR remains dependent on the earlier increments, and the remaining #477 gates
are unchanged.

## Prior increment: frozen intent and guarded envelopes

Depends on [foundation PR #599](https://github.com/Community-Duris/Duris/pull/599)
at `a63ae0d26c4060d21054a6056009b7ece8f8ef60`; review/merge in that order.
This follow-up adds EAI1 frozen intent, bounded schema-2 wire encoding, binding
verification, and explicit legacy SQL entrypoint guards. It does not add storage,
accounting execution or gameplay activation. Existing schema-1 bytes and execution
remain supported. Unsupported durable records must stop coordinator replay without
applying or checkpointing them. See [INTENT_DESIGN.md](INTENT_DESIGN.md).

Required evidence: independent wire fixtures, malformed/capacity/binding rejection,
SQL rejection before connection access, flat-file rejection without root mutation,
legacy-only and mixed unsupported journal recovery, and both server builds.
#475/#476 remain open pending complete contracts and typed transactional adapters.

## Delivered foundation: PR #599 (awaiting review)

The first PR extracts the existing bounded types and plan codec, golden fixtures,
contract model, and writer census onto canonical master
`48c0aedd8e094eee37285111e46e735e4cf12320`. The original integration branch is
preserved. Only the two new pure modules are registered in the server build.
There are no callers, command-envelope changes, stores, migrations or activation.

The types are unchanged from the existing integration branch. The plan codec is
extracted from `a6988d26a`, before frozen-intent/command-envelope integration;
subsequent schema-2 support belongs with its repository admission gates in the
next increment. No independent pure-code bugfix was discarded by that boundary.

Acceptance for this increment:

- Existing golden examples and negative contract tests pass.
- The lexical census matches this source tree and identifies current writer/test
  anchors. Incomplete semantic classifications remain explicit.
- Pure types and plan tests pass under ASan/UBSan; canonical plan bytes agree in
  SQL and flat-file compilation modes. This is not native database qualification.
- Both supported server builds link the new modules without runtime integration.
- Formatting passes for the new C++ files; existing runtime files are unchanged.

**#475 and #476 stay open.** This increment does not freeze every writer policy,
complete semantic site mapping, grant authority through a caller-supplied reason,
or connect the coordinator. A passing structural validator is not proof of
current-state authorization or atomic persistence. The draft release check must
continue refusing incomplete coverage.

## Historical delivery order (superseded)

| Phase | Delivery | Exit gate |
| --- | --- | --- |
| Foundation follow-ups (#475-478) | Finish contract decisions; frozen intent and guarded command envelopes; SQL storage, then flat-file storage and lifecycle registration. Keep shared interfaces serial. | Whole-operation atomic evidence and exact-ID replay on each backend, with no gameplay activation. |
| First complete journey (#479-480) | Controlled maintenance cutover, wallet/bank gameplay producers, commit and publication. | Real commands and restart/retry apply once on both backends. This alone does not complete every holding/source binding in #479. |
| Core coverage (#479-482) | Remaining holdings, currency operations, item custody, grants and costs; start #487 reconciliation. | All core supported writers are covered and holdings/custody reconcile. |
| Domain integrations (#483-486) | Separate shop, collector, auction and death/world/recovery PRs. | Each domain's complete player journeys and reconciliation pass on both backends. |
| Operations (#487-489) | Complete protected audit, guarded corrections, lifecycle/retention and verified restore. | Operator and restore journeys preserve authority, immutable history and replay. |
| Final qualification (#490) | Cross-domain fault matrix, predeclared measured budgets, observation/enforcement and runbooks. | Every original acceptance requirement has current evidence before #474 closes. |

Each phase may use smaller linked PRs when dependencies make that easier to
review. Keep the existing feature branch as the source of reusable work; do not
rewrite it or blindly import its broad persistence changes. Track progress as
component available / gameplay connected / journey qualified.

## Activation and scope controls

- Merging code does not enable accounting. Every writer touching activated
  holdings must be covered; incomplete integrations cannot silently bypass it.
- Use the permitted quiesced maintenance boundary. Resolve or refuse pending and
  unpublished work, prove consistent source capture, and retain restart progress.
  A general online global-freeze framework is not a prerequisite.
- Expand item serialization only for a demonstrated required journey dependency.
- Reuse custody and command authority; do not add a second ledger or queue.
- Preserve gameplay, unsupported refusals and existing aggregate claim storage
  where it meets attribution requirements. No automatic auction reimbursements.
- Canonical master uses migration `0030` for telemetry quarantine. Allocate the
  unpublished accounting migration and update its references on current master
  in the storage PR; never rewrite deployed immutable history. This first PR
  deliberately carries no migration and does not reserve a stale number.
- Run focused checks after coherent changes and integrated checks at their phase
  boundaries. Do not rerun unchanged broad suites without a reason.

The prior 80-140-hour range is a planning allowance for the whole remaining
feature, not a deadline. Re-estimate after the first complete wallet/bank journey.

## Known prerequisite retained for reward integration

The representative flat-file gate harness exposed an existing boon completion
size mismatch under GCC 13 at `-O1`: `BOON_REWARD_RESULT_BYTES` is 2,080 while
`critical_apply_result::result_payload` is 2,048 bytes. The identical diagnostic
reproduces on clean foundation head `a63ae0d26`; this increment does not alter that
helper. Gate tests use `-O0` with ASan/UBSan, matching existing flat-file harnesses;
they do not qualify a successful boon reward. Before enabling reward routes or
archiving those receipts, port and verify the already-preserved complete-result
fix (`27222aea4`) rather than redesigning completion storage. Track this with
#481 and the relevant storage/legacy-receipt integration.

The focused boon prerequisite now has [PR #601](https://github.com/Community-Duris/Duris/pull/601); it remains an independent review/merge dependency for reward integration.

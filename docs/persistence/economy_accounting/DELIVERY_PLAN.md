# Economy accounting phased delivery

Approved direction: 2026-09-21. Parent [#474](https://github.com/Community-Duris/Duris/issues/474).
Linked phase PRs supersede the previous one-final-PR instruction. The full feature
contract and all 16 child issues remain in scope; no phase merge authorizes a live
cutover or production deployment. Request **xander-l** review on each PR.

## Current increment: read-only SQL source capture and normalization

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
See [BANK_ADMISSION.md](BANK_ADMISSION.md). Native coordinator replay qualification
is separate from gameplay publication: replay currently does not restore the
in-memory publication-retention flag. Resolve and test publication/save
acknowledgement across restart before activating wallet/bank producers.

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

## Remaining delivery order

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

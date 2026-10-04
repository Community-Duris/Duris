# Accounting review status — 2026-10-04

Status: **source implementation continues; qualification, activation and release
remain blocked**. `coverage_complete=False`. R1–R8 and every applicable major-plan
acceptance gate remain required. This checkpoint supersedes the October 3 source
preparation status, not its bounded historical test evidence.

## Integrated candidate and ownership

Primary integrated experimental-accounting through
`f7d26eaa721cd3b675c0b0c65009a2535813f400` in `a590fc662` (news only), and
Plan5 through `7a78bb065a7979b8dc8ad2ec49295629d6713906` in `432db98be`.
Both were normal local history-preserving merges authorized by the user.
The later Plan5 restore-coin slice fcdb1afd8 is integrated in8d1be035d; see
[its independent report](PLAN5_RESTORE_COIN_EFFECTS_SLICE_2026-10-04.md).
Its 32 native coin decisions and canonical0056 engine results remain specific
to its native tree d9a9610f3 and consumed scripts/fixtures. Combined-source
qualification is still required. Its final900-second per-engine wrapper differs
from the failed240-second attempt; this does not relax the primary candidate's
original gate budgets or establish complete release-host workload acceptance.
The source milestone candidate through `c6b607d41` includes the ten prerequisite
slices below, plus the later local Plan5 integration described above. Plan5's reports remain evidence of
its own inputs, not of this combined source candidate. Root checkout WIP is preserved.

Primary owns Plans1–4, shared contracts/coordinator, producers, registry/matrix
and activation. The separate codex/accounting-plan5 owner retains independent
reconciliation, operator audit, backup/restore and release qualification.
Parallel specialists use bounded file ownership; primary integrates and reviews.

## Implemented prerequisites, unqualified

| Commit | Result and evidence | Remaining boundary |
| --- | --- | --- |
| `254a0379d` | [Critical publication cleanup](CRITICAL_PUBLICATION_CLEANUP_PREPARATION_2026-10-04.md): allocation-safe identity/fence cleanup and best-effort optional completed-cache bookkeeping. | Native fault calibration and whole-owner proof; supported short-string/cache allocation behavior must not be called an unobserved BEFORE RED. |
| `80692d52b` | [Save execution guard](SAVE_EXECUTION_GUARD_PREPARATION_2026-10-04.md): counted apply/checkpoint permits, prepared registration, exact generation holds and sticky shutdown; ordinary pooled SQL cleanup protected. | Complete mutation census, direct borrowed retirement outer owner, recovery/death-conflict writers, ACK reservation, wake and production startup remain open. |
| `f63079a39` | [Runtime root census](ITEM_ROOT_CENSUS_PREPARATION_2026-10-04.md): bounded complete UID-root snapshot, including contradictory runtime entries. | Serialized runtime observation is neither native SQL proof nor physical graph authority. |
| `0aa0bceeb` | [Authority-bound baseline marker](BASELINE_INITIALIZATION_MARKER_PREPARATION_2026-10-04.md): catalog-v2 initialization identity/opening, 19-image atomic staging and fail-closed missing initialized books. | [Plan5 v2 readers](BASELINE_INITIALIZATION_MARKER_INTERFACE_V2.md), explicit legacy migration and full native boundary qualification remain pending. |
| `fd5683abc` | [Ordinary receipt sealing](ITEM_PUBLICATION_RECEIPT_PREPARATION_2026-10-04.md): definitive proof sealed before effects; reentrant/batch contradictions retained; completed publication survives conflicts for ACK-only canonical repair. | Actorless native graph enrollment/publication, epoch proof and full cold recovery remain open. |
| `64c495eb0` | [Nonfatal inert allocation](INERT_ALLOCATION_PREPARATION_2026-10-04.md): no-growth pooled slot acquisition and checked debug-compatible storage. | Exhausted pools retain work; lifetime/allocator profiles and native fault qualification pending. |
| `b36e9690f` | [Discard-only inert literal stage](INERT_ITEM_STAGE_PREPARATION_2026-10-04.md): exact retained UID/text/fields and callback-free partial cleanup. | Prepared-prototype provenance, unsupported behavior, current graph/epoch proof and final enrollment remain open. |
| `2b4591c21` | [Staged lifecycle composition](FLATFILE_LIFECYCLE_COMPOSITION_PREPARATION_2026-10-04.md): private sealed after-image view, complete revision chain and precommit receipt construction. | External boundary fields are assertions; actual native boundary/census, selected-epoch exact retry and activation remain open. |
| `aa385e73b` | [Exact-request sticky wake](DEFERRED_SAVE_WAKE_PREPARATION_2026-10-04.md): allocation-free before-park notification and guarded notified dispatch. | Actual guard/persistence timing, replay revisit and production recovery remain open. |
| `c6b607d41` | [Original hold wake handoff](DROP_HOLD_WAKE_HANDOFF_PREPARATION_2026-10-04.md): capture active identity before exact guard release; notify after pipeline unlock. | No production post-drop ACK caller; complete mutation census, ACK reservation and cold startup remain open. |

Source reviewers found no remaining blocker in each slice's stated narrow scope.
This is source review, not native/build/gameplay/persistence/recovery acceptance.
Existing healthy inactive behavior and the declined spell change are preserved.

## Prepared evidence and execution cadence

The user requests testing when each major plan is ready. This checkpoint runs no
new compiler, AST, native, SQL, gameplay, migration, service or recovery checks.
Only source review, formatting and diff hygiene were performed. New source commits
are unqualified and have not been pushed as qualified milestones.

Private frozen inputs and cases are prepared, not executed:
critical cleanup seven groups; save guard 35 leaf and 14 pipeline cases; runtime
root census ten groups; marker 546-input closures; receipt publication 30 unchanged
craft cases plus 19 ordinary cases; allocator eight cases; discard-only constructor
22 groups; lifecycle13 cases in corrected private V2; worker wake13 cases with533 inputs
per corrected V2 variant. Exact pins and controlled-leaf
limits are in
the linked reports. A missing BEFORE API/compile failure is unsupported, not a
semantic RED. Original case/aggregate/compile budgets and maintained oracles remain.
Later combined-candidate qualification must refresh consumed source pins; these
isolated frozen families do not establish current-head acceptance or route coverage.

## Remaining integration

Follow the [ordinary drop recovery interface](ORDINARY_DROP_RECOVERY_INTERFACE_2026-10-04.md):
prepare closed; restore original holds; start guarded execution; independently prove
native epoch/payload/graph; materialize without economic side effects; establish a
clean mutation census and ACK reservation; checkpoint critical publication; clear
only the exact original hold; retain wake across parking. Production comm still
uses immediate save init before critical replay; the new APIs alone do not repair
that production sequence. No login bypass, blind resume or quarantine bypass.

The [Plan5 handoff](PLAN5_INTEGRATION_HANDOFF_2026-10-04.md) records source-traced
stake retirement and retained-price metadata gaps, native marker-v2 reader contract,
and missing actual snapshot-exception/death-conflict matrix adapters. Counterexamples
are predictions, not executed failures. Plan5 owns those independent changes.

Plans2–4 still require the remaining native ordinary/compound writers, producer
integration, precise refusal for unsupported active routes, semantic writer
reanchoring and executable same-root coverage. Historical lexical inventory counts
and synthetic fixtures do not complete accounting. Plan1 lifecycle, baseline,
activation, pause and recovery and R7/R8 independent release gates remain required.
Qualification must cover both SQL engines, flatfile, actual gameplay, populated
upgrade, replay/lost reply/restart/copyover and measured workloads on one candidate.
No force push, PR merge, deployment, production activation or production data changes.


## Later source follow-up

Semantic registry preparation442c90d31 classifies inert private allocation and
cleanup as nonwriters;54839ae8e restores all prior row data/escaping after a detected
serialization change. The generated matrix and historical census/source anchors
remain untouched. Backend evidence stays unverified. No generator, AST or tests ran.
Staged lifecycle composition now exists; older pending-composition reports refer
to the marker-only scope. Native boundary
assertions, independent v2 readers, migration, all holdings/item-source coverage,
production recovery/activation and final candidate qualification are still required.


## Exact-request deferred wake follow-up (source only)

The worker now retains a notification on the exact active request, including a
notification before parking. Monotonic request/lifecycle identities prevent credit
from passing to a replacement; notified parked dispatch requires no ready-queue
allocation, rechecks the execution guard and does not poll if still held. Ordinary
FIFO and notified work alternate when both are available. The primary post-drop
hold owner captures that identity before exact hold release and notifies it after
unlock. Failed hold release restores the original checkpoint and sends no notice.

See [worker preparation](DEFERRED_SAVE_WAKE_PREPARATION_2026-10-04.md) and
[shared handoff](DROP_HOLD_WAKE_HANDOFF_PREPARATION_2026-10-04.md). Source review
found no blocker within this mechanism's scope. New source remains UNQUALIFIED;
no compiler/native/SQL/gameplay/service/recovery tests or milestone pushes ran.
Legacy PID-only wake now reports acceptance, including coalesced duplicates;
BEFORE retry/race/OOM fixtures must remain preserved with explicit new AFTER
semantics. No missing API/compile failure counts as semantic RED.

No production caller of the post-drop acknowledgement API exists yet. Worker wake
does not revisit journal replay, prove a clean affected-PID mutation census, reserve
critical ACK or authorize current epoch/native graph construction. Those owners,
actual producer/gameplay integration, semantic matrix reanchoring and all R1-R8
acceptance gates remain open. The recovery interface now records the next bounded
actor-independent existing-graph verifier and later all-absent enrollment contract.

The worker V1 preparation accidentally overlaid the AFTER header into BEFORE.
Original V1 is preserved as insufficient; corrected V2 restores the exact header.
Primary raw inventory review confirms533 files per variant and only worker.c/h
differ, with exact frozen/current source matches. The report correction follows
aa385e73b; no native execution or source change follows from that metadata check.

# Economy accounting phased delivery

Approved direction: 2026-09-21. Parent [#474](https://github.com/Community-Duris/Duris/issues/474).
Linked phase PRs supersede the previous one-final-PR instruction. The full feature
contract and all 16 child issues remain in scope; no phase merge authorizes a live
cutover or production deployment. Request **xander-l** review on each PR.

## First increment: reviewable pure foundation

This PR extracts the existing bounded types and plan codec, golden fixtures,
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

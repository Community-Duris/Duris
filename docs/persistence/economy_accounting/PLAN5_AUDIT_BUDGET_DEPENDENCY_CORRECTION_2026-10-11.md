# Plan 5 audit-budget preparation dependency correction — 2026-10-11

The initial three-file ACC-05 preparation omitted the independent retained-plan
decoder `scripts/economic_restore_evidence.py`. The actual selected audit path
refuses with `ModuleNotFoundError` before budget qualification. The corrected
preparation freezes that fourth exact primary file; small price and mapping
dependency probes pass with unchanged inputs and one original plan verified
each. Both original Linux budget methods remain **NOT_RUN**, with no resources
started or grant used.

This report supersedes the dependency-closure and three-file claims in
[the initial preparation](PLAN5_AUDIT_BUDGET_PREPARATION_2026-10-11.md). Its original
receipt, inputs, helpers, publication and raw proof are preserved under
`preparation-v1`; the initial remote report commit remains preserved. The native
tests, assertions, measurement function, selector argv, limits, image, mounts,
resource/cleanup contract and component-only scope are unchanged.

## Exact source and defect

- Branch/destination: `codex/accounting-plan5`; worktree
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Producer base: `30ea24a588a5a331bb8a8fb002ae852990f9a779`.
- Exact proposed component source remains
  `0555741d740cf028552fcf63973265bfbcda052c`, tree
  `36ff31fdd214ca80c69b5edf2c4aabcc1f69b2ef`.
- Maintained files: this new correction report and additive remote follow-up
  only. No shared interface/schema request, reader/test/migration/registry or
  mutation change. Result/verified remote identity is bound after publication
  in `delivery/preparation-publication.v2.json`.

The original reader's `valid_item_custody_position` imports `EvidenceError` and
`valid_position` from the decoder at line186. `audit_original_plans` imports
`MAX_PLAN` and `decode_plan` at line861. Both selected fixture paths audit retained
EAP1 plans. Freezing just reader/tests/registry therefore cannot execute these
tests. The decoder's top-level imports are standard-library modules and the
already frozen reader; its deferred `economic_sql_audit_origins` import belongs
to `verify_canonical_baseline`, outside this selected path. That unrelated SQL
module is not added to the freeze.

The four final inputs are original Git blobs. The first three SHA-256 identities
remain exact:

| Input | SHA-256 |
| --- | --- |
| `scripts/reconcile_economy_accounting.py` | `f8235cab0f79e785d8a27183ac0076a44e334a46afffb181c5e83958f28b1dd5` |
| `tests/async/test_reconcile_economy_accounting.py` | `ba7cb643673bf445c625c243313ec4e498213d3a52e73d909cbad6ebbca3c9c9` |
| `docs/persistence/economy_accounting/registry.json` | `fbe53c9a037fe3bcdcb27cfed4110ed39e01c6acaa1917c26c34ce957bf0e8df` |
| `scripts/economic_restore_evidence.py` | `bb927db1a509d377e9930b264534665f38bd88ebbbee5672075db7267ad6f0bc` |

The added decoder Git blob is `06a2ebeaddd4bf604990b0f6cacf0c842555ccf3`.
Final source inventory SHA-256:
`85161510b291142e388ccdb33c7466a9a63fac8432bb769add79bd1358024267`;
four-file archive SHA-256:
`1a206ab72fd37ddff471981b627a911eb36202bab4d904f030e972b6a66a04ab`.

## Actual resource-free proof

Host Python3.12.10 runs the external `dependency-proof/probe.py` against the
preserved three-file source and the corrected four-file source. It imports the
unchanged test module, constructs its small original `price_snapshot()` and
`mapping_creation_snapshot(1)`, calls the original reconciler and bounded price
view, and checks input identity. It does not execute a budget selector, generate
a large input, launch a CLI, start Linux/container/database/server work or claim
acceptance evidence.

Before correction, the first small price audit exits1 at reader line186 with
`No module named 'economic_restore_evidence'`. After correction, exit0/empty
stderr; both one-operation probes verify exactly one retained original plan,
report no exception and preserve their input bytes. The loaded modules from
the frozen source are exactly the original reader and decoder. The deferred
SQL module is absent. Commands, raw before/after logs, program and structured
result are retained in `dependency-proof`.

The first correction-helper attempt passed both small probes but then failed
its own module-enumeration expectation: it included `__main__` despite the probe
being outside the frozen source root. The actual local module set was correctly
two. The original helper/logs are preserved in `dependency-proof/first-attempt`;
the corrected expectation and focused probe retry pass. This helper failure
does not change a native result; no native run occurred.

A second metadata correction updates check-only stdout from `source_files:3`
to4, matching its already corrected four-file guard and inventory. Prior metadata
is preserved in `dependency-proof/metadata-before-count-correction`. Final
`--check-preparation` exits0/reports4, and missing-grant execution exits1 with the
original pre-Docker refusal. All source/assertion/native-command bytes remain
exact. Transport and seal counts now both bind four source files.

Final receipt/helper/result hashes:

| Artifact | SHA-256 |
| --- | --- |
| `preparation-receipt.json` | `6f5189db7e1bab5c101d4c669cf2b5e997b9c02b86c58c83ec8e6b0c0e9721bb` |
| `helpers/run_audit_budget.py` | `b3303bd954ec8d07bd390d3395c67948960bfe400039b28bc0ca14fac8e7985e` |
| `helpers/seal_audit_budget.py` | `414b71eb0d505900347d46efdfdac5b70368780337e2530a8b293e58542fe502` |
| `dependency-proof/result.json` | `3604d849198dbfd95f10842919e354cb6fb1250814f1db6b49dfb997c2316854` |

Evidence root remains
`D:/Dev/Tests/Duris/accounting-plan5/ACC-05-AUDIT-BUDGET-01`.

## Pending native run and curator handoff

The unchanged original native command is:

```text
python3 -u -B tests/async/test_reconcile_economy_accounting.py AuditBudgetTests.test_near_limit_price_view_cli_budget_and_limit_invariance AuditBudgetTests.test_near_limit_mapping_snapshot_cli_budget_and_limit_invariance -v
```

Run only after a distinct grant pins the final hashes above and matches the
exclusive global lease. Preserve the initial report's exact single foreground
QA image74b699...,2GiB/CPU2/600s, direct D: source/scratch/evidence mounts,
networknone/read-onlyroot, explicit budget flag, twelve original budget records,
two original oversized-refusal assertions, zero skips, sealed terminal evidence,
exact owned cleanup, four passive volumes/zero consumers and seven unrelated
live IDs. No observer or test/measurement patching is added.

Only preparation dependency closure is corrected. Both Linux methods are still
NOT_RUN. Their eventual pass would remain a synthetic offline component, with
release_host_qualified false. Complete two-engine65/startup/tamper/contracts,
current backup/restore/retention/durable erasure, wallet/room/native authority,
recovery/gameplay/load/mixed32MiB/host and R1–R8 release gates stay OPEN.

This additive correction/follow-up is the primary-local notebook curator packet;
no notebook import is claimed. Accounting remains inactive/CLOSED, coverage
incomplete, release BLOCKED and full goal ACTIVE. Existing work/history/evidence,
wallet-root item exclusions and declined inactive spell behavior are preserved.
No activation, deployment, merge, production mutation or audit autocorrection.

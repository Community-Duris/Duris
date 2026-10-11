# Plan 5 current-primary Linux audit-budget preparation — 2026-10-11

ACC-05-AUDIT-BUDGET-01 is prepared and **NOT_RUN**. It selects the two existing
Linux AuditBudgetTests on exact primary source0555741d7. Preparation starts no
container, native test, database, server or build. A distinct coordinator grant
and matching exclusive resource lease are required before execution. Earlier
f78/current61 budget evidence retains its historical scope.

## Source, branch and ownership

- Owned branch and publication destination: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Producer base: `b8505d97bc6e4bccbe03ce73085416f1345d7951`.
- Refreshed primary and exact proposed component source:
  `0555741d740cf028552fcf63973265bfbcda052c`, tree
  `36ff31fdd214ca80c69b5edf2c4aabcc1f69b2ef`.
- Result/remote/report identities are bound after publication in the owned
  `delivery/preparation-publication.json`; the source above remains the proposed
  test source, independently of this report's producer commit.
- Maintained changes: this report and additive
  `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md` only. No shared interface or schema request.
  The original reader, tests, registry, assertions and native measurement helper
  have **zero changes**. Broad unadopted Plan5 changes do not enter this component.
- All seven historical Plan5 tips remain ancestors. Existing branches,
  worktrees, dirty files, failures, storage profiles and evidence are preserved.

The three actual inputs are copied from Git blobs into a fresh D: source layout;
no live checkout, environment, credential, player data or operational archive is
mounted. The reader imports standard-library modules and reads the original
registry. Local optional/native test imports elsewhere in the test file are
outside the two selected methods and their helpers.

| Frozen input | SHA-256 | Git blob |
| --- | --- | --- |
| `scripts/reconcile_economy_accounting.py` | `f8235cab0f79e785d8a27183ac0076a44e334a46afffb181c5e83958f28b1dd5` | `61a89d5e00a8653decb850dec81e450f9507cf3d` |
| `tests/async/test_reconcile_economy_accounting.py` | `ba7cb643673bf445c625c243313ec4e498213d3a52e73d909cbad6ebbca3c9c9` | `72c70173f53cf203e171fa35c5553c566b58b860` |
| `docs/persistence/economy_accounting/registry.json` | `fbe53c9a037fe3bcdcb27cfed4110ed39e01c6acaa1917c26c34ce957bf0e8df` | `f3797ec71cd79c7f24fb0f3c015af6ef5ad39754` |

Source inventory SHA-256:
`314b07d853aa7eaaa6d8760e012103d8df9ea8b132fd61e7a21d853c8640f24b`.
Three-file archive SHA-256:
`f3d64cd1a2ce0470e6f568fcd2bbb8df60bf5ed848ee693ae9c817b3cfed3a87`.

AGENTS.md, current Plan5, finish/remaining requirements, latest review checkpoint
and reduced three-session charter were refreshed. No tracked AI_CONTEXT.md is
present in the current primary tree, and neither owned nor primary checkout root
contains that file. This is an observed missing context file, not a shared
notebook blocker. The primary-local notebook remains the curator destination.

## Original executable acceptance

Run only these original methods, with explicit Linux budget opt-in:

```text
python3 -u -B tests/async/test_reconcile_economy_accounting.py AuditBudgetTests.test_near_limit_price_view_cli_budget_and_limit_invariance AuditBudgetTests.test_near_limit_mapping_snapshot_cli_budget_and_limit_invariance -v
```

`DURIS_RUN_AUDIT_BUDGET=1`, `PYTHONDONTWRITEBYTECODE=1`, and
`TMPDIR=TEMP=TMP=/scratch` are set in the container. The native command invokes
the original unittest entry point directly. There is no observer, patched test,
replacement measurement, success handling, timeout handling or skipped selector.
The original `measure_audit_cli` fresh-child subprocess and RUSAGE timing/memory
path remain exact.

Acceptance requires actual terminal0, two methods/no skips and all twelve
original measurement records: price clean/conflicting and mapping clean/corrupt,
each at detail limits0/1/100. Preserve every original 32MiB/100000-row/100-output,
30-second and256MiB assertion, real CLI subprocess, global-count invariance and
file-immutability check. Both original oversized-input checks at limits0/100 must
complete, refusing with exit2, empty stdout, the exact original diagnostic and
unchanged input. Their proof is the unchanged mapping method's assertions and
its actual successful native terminal; no fabricated oversized records are added.

The original price fixture can reach the row bound before its byte bound and
pads with valid JSON whitespace. Record actual rows/structured input/padding
without presenting it as a complete mixed native snapshot. Original raw stdout,
stderr, command, receipt and immutable source are authoritative. The host seal
parser verifies the full twelve-record matrix and original unittest terminal.
An actual failure remains failed with its exact logs; it is never converted to
a pass or silently retried.

This is a synthetic offline JSON component. It cannot qualify complete native
SQL/flatfile authority, a running game, the release host or full accounting.

## Proposed resources, execution and cleanup

Pinned QA image:
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`.
One fresh foreground QA container, `--rm`, `--restart no`, networknone,
read-only root/source, maximum2GiB/CPU2,600-second outer bound. Direct D: mounts:

- `D:/Dev/Tests/Duris/accounting-plan5/ACC-05-AUDIT-BUDGET-01/source` to
  `/opt/duris`, read-only.
- The evidence root's fresh `native` directory to `/evidence`, writable.
- `D:/Dev/Temp/accounting-plan5-ACC05-20261011/scratch` to `/scratch`, writable.

The task build path is
`D:/Dev/Builds/Duris/accounting-plan5/ACC-05-AUDIT-BUDGET-01/bin`; no build or
build-cache use is proposed. Read-only preparation inspected compatible slots
and current jobs: no `duris-build-a/b` slot exists, seven unrelated services are
live, the pinned image is available, and the exclusive global lock is absent.
Their full IDs and all existing container entries are retained in the evidence
inventory. Names alone never authorize reuse or stopping another owner's job.

Four passive owned database volumes are retained with zero consumers at
preparation: `plan5-acc02-maria-fs01-linux-data`,
`plan5-acc02-retry03-be1b6ef31b-mariadb-data`,
`plan5-schema65-diag01-fc7f493367-data`, and
`plan5-schema65-diag02-7bdf82089d-data`. This ticket mounts no database volume.

The transport verifies the distinct reviewed grant, preparation and helper
hashes, exact owner/job/source and GRANTED global lease before Docker actions.
It records the actual container identity, labels, limits and mounts; finally
cleanup operates only on the exact owned identity. Abnormal termination captures
the failure and stops only that owned job. Native/foreground terminal, successful
empty exact-owned inventory, unchanged source, all four passive volumes without
consumers and preserved unrelated live IDs are required before sealing and
guarded release of the matching global lease. Cleanup failure remains unresolved
with recovery resources/lease preserved. No system pruning or unrelated stop.

Proposed host command after grant:

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B D:/Dev/Tests/Duris/accounting-plan5/ACC-05-AUDIT-BUDGET-01/helpers/run_audit_budget.py --preparation D:/Dev/Tests/Duris/accounting-plan5/ACC-05-AUDIT-BUDGET-01/preparation-receipt.json --grant D:/Dev/Temp/accounting-acceleration-bootstrap-20261011/ACC-05-AUDIT-BUDGET-01-resource-grant.json
```

Then run `helpers/seal_audit_budget.py --preparation` against that same receipt
only after the original process has actually terminated and cleanup succeeds.

## Preparation proof, remaining gates and curator packet

Evidence root:
`D:/Dev/Tests/Duris/accounting-plan5/ACC-05-AUDIT-BUDGET-01`.
Preparation receipt SHA-256:
`9fc68d0ea747907eb8391aac9239746f463b2302468353433a4b8b652b2f9786`.
Transport SHA-256:
`251ce10dd5c7c0d9913273a260b33cff35c876ad3af53aeed18a16dfedef11ef`.
Seal helper SHA-256:
`6191f10f1346ea2277bb4c0c1188553f7af3d51aff51945e0cdce93eb2dc8789`.

Windows Python3.12.10 preparation copies exact blobs, verifies all three source
files/archive/helper hashes, parses all three helpers and both selected methods,
and reproduces successful `--check-preparation`. Missing grant refuses with
exit1 and `distinct reviewed grant required before Docker actions`, before any
Docker action, job receipt, native directory, CID or lease. Commands and raw
outputs are retained in `preparation-proof.json` and matching logs. These are
resource-free preparation checks, not execution of either Linux method.
No skips or native passes are claimed; both Linux methods remain **NOT_RUN**.

Plan ablation removed the unexecuted observation proposal and any full-suite,
SQL, server or compilation work. Only exact source freeze, direct original
command, resource/cleanup transport and terminal evidence seal remain.

Remaining gates: this exact Linux run/grant; complete current two-engine65
matrix, startup/tamper and measured-contract integration; current backup/restore,
native retention, durable erasure propagation and replay; wallet/room authority;
complete mixed32MiB/host/gameplay/fault/load and R1–R8 release acceptance. No
interface, schema, producer, registry/matrix, activation or mutation repair is
requested. Earlier isolated passes and inventories do not establish release.

This additive report/follow-up is curator-ready input for the primary-local
shared notebook; no import or acknowledgment is claimed. Accounting remains
inactive, admission CLOSED, coverage incomplete, release BLOCKED and full goal
ACTIVE. Wallet-root item exclusions and the declined inactive spell-path change
remain preserved. No production data, authority, activation, deployment, merge
or audit autocorrection occurs.

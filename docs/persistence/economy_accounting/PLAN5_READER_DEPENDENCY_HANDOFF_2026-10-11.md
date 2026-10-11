# Plan 5 reader transport dependency cut — 2026-10-11

The independent reader fixes can be integrated into the current acceleration
candidate without importing the broader Plan 5 branch. The narrow patch changes
only the CLI transport and four regression methods. It passed the five applicable
Windows checks; the POSIX FIFO method remains a platform skip. This is component
qualification, not native protocol or release acceptance.

## Branch, source and exact dependency

Owned branch/remote: `codex/accounting-plan5`. Worktree:
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
This documentation-only slice starts at `fcefe3a1e54d68694a2a3b573e09a53bb31d914c`. The result SHA is recorded in
the commit and post-publication delivery receipt. The patch is
[PLAN5_READER_TRANSPORT_MINIMAL_2026-10-11.patch](PLAN5_READER_TRANSPORT_MINIMAL_2026-10-11.patch).

The exact tested base is orchestrator candidate `e153862c9bf47e6cc1300d63d438eb9dd8147053`; its current reconciliation
logic, registry, fixtures and 138 existing test methods are preserved. Tested
composition tree: `9e5b8332c6a5213bdcada82d717101d0f0cb2cb0`. Only two files overlay that base:
`scripts/reconcile_economy_accounting.py` and
`tests/async/test_reconcile_economy_accounting.py`. All remaining AST, including
the Reconciler class, is exact to the base. `git apply --check` passes against the
full unmodified D: export. No maintained runtime/migration/registry file changes.

Historical prerequisites, each already visible on the owned remote branch:

- `0a16ba2187ecfc77b6306c0e3f9a3be5d85eeb4e`: reject duplicate JSON keys at every
  object through `unique_fields`, before auditing or producing any view.
- `f7203b4c438b35da1f1d1fcf3267d3e740a65ada`: import `os`/`stat`, open a descriptor
  without waiting for a FIFO writer, reject nonregular input, check descriptor
  size, read at most `MAX_INPUT_BYTES + 1`, reject growth past the bound and release
  the descriptor on every path.
- `fcefe3a1e54d68694a2a3b573e09a53bb31d914c`: normalize only a JSON parser `RecursionError` into the existing refusal
  path. Internal reconciliation errors are outside this catch.

Port the attached transport patch rather than cherry-picking whole historical
files. Preserve the candidate's domain audit logic. The patch retains seven views,
the input/output bounds, and success/finding/refusal exit statuses 0/1/2.

## Actual checks and evidence

Backend: Windows CPython 3.12.10; executable SHA256
`4d6f5f81a4bca11191c4c7c6b43632694d0a4ce74e068619d8fdc161d469859a`. From the projected source root:

```text
python.exe -B tests/async/test_reconcile_economy_accounting.py
  ReconciliationTests.test_cli_refuses_nonregular_snapshot_without_waiting
  ReconciliationTests.test_snapshot_growth_after_size_check_cannot_bypass_byte_limit
  ReconciliationTests.test_cli_rejects_duplicate_snapshot_fields_before_any_view
  ReconciliationTests.test_cli_refuses_excessive_json_nesting_before_any_view
  ReconciliationTests.test_oversized_snapshot_and_collection_refuse
  ReconciliationTests.test_cli_bounded_exception_result -v
```

Six methods in 27.911 seconds: five PASS, one POSIX FIFO skip, zero failures/errors, exit 0.
The duplicate-key and nesting methods invoke actual CLI subprocesses across all
seven views and limits 0/1/100; nesting includes 42 controls. Growth uses the
original descriptor-size race injection. The existing controls preserve valid
exit 0, audit finding exit 1, refusal exit 2 and bounded collection/output behavior.
No SQL, C++ build, gameplay or mutable service was used. All source bytes match
their before-run guards after execution.

Evidence: `D:/Dev/Tests/Duris/accounting-plan5/reader-dependencies-20261011-v3`.
Patch SHA256 `dc40da057902e80ed0cb6a68407abb4c80e7bdc5c927d8e5e1691200446f80d9`; component archive SHA256
`19747a875d62f1002f3e87d57b36100b7074b4aec133f7e0b1aeb25efd342c56`. Test stderr SHA256
`b4d565b6822d178165ee7c6d42977ac4f13cb75228cfdc80856f2588fc76cb94`. Sealed manifest SHA256 `da92961970b26d9bbb43d57f4b485b41236e55b3285fe2b27d5d4926bc2c4947`,
2543 files, 73321540 bytes; immutable members include the
source export, projection, raw results and helper sources. The post-publication
delivery receipt is outside those immutable members.

Earlier preparation attempts are retained under the adjacent unsuffixed and `-v2`
directories. The first projection omitted a FIFO decorator and was not executed.
The second omitted the current registry documentation from its component archive:
21 failures and two errors, one platform skip, were caused by that missing file.
The third adds the base's exact accounting docs and preserves the decorator;
production/test patch bytes are identical to the second projection. These are
recorded harness preparation errors, not hidden passing claims.

## Shared owner handoff and next accepting gate

No new schema or accounting interface is requested by this slice. The primary
owns integration registration and source-pin refresh. The previously requested
`plan5_recent_item_history` row should include the nesting method with its required
case/arguments while preserving existing controls; register the reader transport
methods according to their existing platform contract. Never suppress the FIFO
skip or label it as executed on Windows.

The human-approved acceleration transition arrived before the earlier planned
hold. The full Plan 5 goal remains ACTIVE; no pause was enacted. ACC-02 setup is
authorized without a heavy start while ACC-01 owns the build slot. The separate
unmodified D: matrix export is prepared on `e153862c9bf47e6cc1300d63d438eb9dd8147053` without this parser projection.
Resource grant, exact candidate selection and binary pins remain with the
orchestrator. No new container was started or lease acquired here.

Schema 65 actual two-engine measurement/default and explicit startup, original
backup/restore, native first admission and wallet/room authority, retention/erasure,
recovery/fault/load and R1–R8 release gates remain OPEN. A projected reader pass
does not qualify them. Accounting stays inactive/admission CLOSED. The shared
notebook remains primary-local and nonblocking; this report/follow-up are additive
curator input, without a claimed curator import or primary acknowledgment.

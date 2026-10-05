# Plan 5 refreshed maintained build and contract handoff

Both fresh maintained server builds stop at three omitted aggregate members in
the primary-owned item movement producer. The previously reported SHOP epoch
compile error and both SHOP test anchors are repaired in the consumed primary.
All 71 original contract methods now pass. These results do not qualify release.

## Source and delivery

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `f325e19806cb184baa0b1b769d35b53066c06c1f`, the preserving merge of
  refreshed primary `a0153060c8a15e5770a45c3ce52fc51e727f5869` into the requested
  branch. Previous published Plan 5 head was
  `fd0a6f2e4e73acdecbed4798b341e66e3e84374b`.
- Native tree: `cfe6d1c0cefd8539aa2754faf8a94b7ed2aefd48`.
- Migrations tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, canonical0056.
- This report is the only owned file in this slice. Result SHA and verified
  remote head are recorded in `tmp/plan5/a015-build-delivery.json`.
- Evidence manifest: `tmp/plan5/a015-build-evidence.json`. Existing old Plan 5
  branches and their completed commits remain preserved in the requested
  branch's ancestry; `preserved-branches.json` records the renewed checks.

No shared producer, coordinator, schema, contract, registry, matrix or activation
file was independently edited. The pending separate saved-reconciler fix was
present during contract tests; exact source preimages/hashes distinguish these
Python tests from the native builds. Existing branches and historical evidence
are retained rather than relabeled as current qualification.

## Exact native builds

Both builds used immutable local Linux image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
network disabled, a read-only workspace and writable `bin/`. Each selected a new
empty object directory and isolated `BIN_ROOT`; no old objects or backend stamps
were reused. The maintained Makefile's complete warning/hardening profile was
kept, including `-Werror=missing-field-initializers` through `-Werror`.

```sh
make -C src -j2 PERSISTENCE_BACKEND=mariadb \
  BIN_ROOT=/workspace/bin/tests/p5-a015-build-20261005/bin-sql \
  OBJDIR=/workspace/bin/tests/p5-a015-build-20261005/objects-sql \
  DMS_BINARY=/workspace/bin/tests/p5-a015-build-20261005/server-sql
make -C src -j2 PERSISTENCE_BACKEND=flatfile \
  BIN_ROOT=/workspace/bin/tests/p5-a015-build-20261005/bin-flatfile \
  OBJDIR=/workspace/bin/tests/p5-a015-build-20261005/objects-flatfile \
  DMS_BINARY=/workspace/bin/tests/p5-a015-build-20261005/server-flatfile
```

| Backend | Exit | Seconds | Compile commands | Objects | Errors | Linked server |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| SQL policy | 2 | 257.974 | 380 | 379 | 3 | No |
| Flatfile policy | 2 | 270.472 | 380 | 379 | 3 | No |

There were no separate `warning:` diagnostics. All three errors are the
warning-as-error diagnostic for missing `item_transfer_payload::native_mobile`.
All `src/` and migration input hashes remained unchanged during each build.
Logs, complete compile lists and input maps are in
`bin/tests/p5-a015-build-20261005/build-{sql,flatfile}-{inputs,results}.json`
and `build-{sql,flatfile}.log`. Unlinked objects remain in their original
namespaces; they are not successful executable qualification artifacts.

## Narrow primary-owned fix request

The existing member is `item_native_mobile_context native_mobile`, declared at
`src/item/item_transfer_command.h:247`, after `continuation`. Its absence is
intentional on ordinary movement, batch and craft routes; the default context
has `present=false`, the reference's existing default initializer, zero action
and zero final giver. Explicit `.native_mobile = {}` after `continuation` in the following
aggregates should preserve that invariant and the original field order:

| Consumer | Aggregate end | Diagnostic |
| --- | ---: | --- |
| `submit_movement` | `src/item/item_movement_transaction.c:2580` | Missing member |
| `item_movement_transaction_submit_batch` | Same file, line 3082 | Missing member |
| `item_movement_transaction_submit_craft` | Same file, line 3403 | Missing member |

No public interface, wire version, schema field, producer authority or activation
change is requested. The primary owns the implementation and must retain the
native context's absent semantics, the dormant/admission refusals, inactive
behavior, wallet-root item exclusions and declined inactive spell-path change.
Re-run these two original maintained commands after the fix, then the relevant
movement/batch/craft context and inactive-path tests. A partial compile or a
warning suppression does not close this gate.

## Contract checks and remaining gates

```sh
PYTHONPATH=tests/async python3 -u -B -m unittest -v \
  test_economy_writer_coverage_contract test_audit_accounting_invariants
python3 -u -B scripts/validate_economy_accounting.py
python3 -u -B scripts/generate_economy_writer_coverage.py --check
python3 -u -B scripts/validate_economy_accounting.py --release
```

The original 71-method run passed with no skips in 20.394 seconds including
source capture, 11.431 seconds in unittest. Both ordinary validator and matrix
check exited 0. The release validator exited 1 as required with
`writer has no executable evidence`. Current inventory is 14 fixtures, 897
registry rows and 2,862 candidate occurrences; the checked matrix explicitly
retains `coverage_complete=false` and `release=BLOCKED`. Inventory is evidence
of source bookkeeping only.

Contract logs/input maps are in
`bin/tests/p5-saved-integer-20261005/contracts-current*`; validator commands,
results, logs and unchanged-source receipt are under the build namespace's
`validators/`. No live or production database was used by these checks. No
gameplay boot or managed restore was attempted with an unlinked candidate.
Those executable gates remain blocked by this reproduced shared build defect.
Full producer capture, combined-candidate R1–R8, backup/restore/retention and
release qualification remain open.

This report is a curator handoff for the primary's locally maintained notebook.
Notebook locality is not a blocker. Merge the completed report from the requested
remote branch, record the exact consumed source and preserve the original failed
build evidence. The primary owns central registration and combined qualification.

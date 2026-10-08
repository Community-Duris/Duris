# Plan 5 original-plan projection representation qualification

The saved-snapshot reader could report a native plan as verified after an integer
field was replaced by an equal float or Boolean. This complete owned fix compares
both the type and value of all original-plan detail projections. It retains the
existing mismatch findings and never repairs or mutates the supplied evidence.
Release remains incomplete.

## Source, delivery and ownership

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `72fee509ca517c4ab422f53e49bf145493d99d11`.
- Refreshed primary consumed: `a0153060c8a15e5770a45c3ce52fc51e727f5869`, through
  preserving merge `f325e19806cb184baa0b1b769d35b53066c06c1f`.
- Native tree: `cfe6d1c0cefd8539aa2754faf8a94b7ed2aefd48`.
- Migrations tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, canonical0056.
- Result SHA and remote verification: `tmp/plan5/saved-integer-delivery.json`.
- Evidence manifest: `tmp/plan5/saved-integer-evidence.json`.

Owned files are `scripts/reconcile_economy_accounting.py`,
`tests/async/test_plan5_child_identity.py`,
`tests/async/test_economic_sql_canonical_audit.py`, `AUDIT_OPERATIONS.md` and this
report. No shared coordinator, accounting contract, producer, schema, registry,
matrix or activation file was independently edited. The operator documentation
also corrects its stale claim that the new exporter omitted original plans:
EAP1 is retained; opaque EAI1 intent facts remain omitted.

## Established defect and fix

The unchanged reader at SHA-256
`a745f1851c9aad64873d0a1f2a79a5d74e902749f09a7d60506316300181f334`
reported fourteen altered model projections clean, with one verified original
plan each. New focused regressions ran against that source before the fix:
18 methods executed, 15 intended subtest failures, exit 1. Original source,
tests, runner, command, input map and red observations remain under the new
namespace, including the exact recorded runner preimage.

The defect is also reproduced on retained native-generated EAI1/EAP1 plans
exported from canonical0056 MariaDB and MySQL fixtures. The preserved old reader
reports all 54 equal-value aliases clean across default, native-mobile and
source-identity variants; the fixed reader detects all 54 with zero verified
plans. This comparison only redirects the relocated old module's registry path
to the unchanged canonical registry. Neither reader's source bytes are altered.
The first comparison harness stopped before auditing because its relocated
registry path was absent; its preimage and failed receipt are preserved too.

`same_projection` uses exact Python types, exact values and recursively equal
list lengths. Its expected values come from the independently validated native
EAP1 decoder. Equality to those bounded native integers establishes their ranges
without duplicating native width tables or coercing JSON values. Recursion is
bounded by the shallow expected projection shape. The five families are:

| Projection | Authenticated fields |
| --- | --- |
| Account effects | Account index/key, coin vectors, before/after revisions |
| Postings | Line/event/account/child indices, coin deltas, signed copper value |
| Children | Index, derived ID, domain/discriminator, parent index, relationship |
| Item references | Line/event/child indices, UID, before/after revisions |
| Linked custody | UID, root, nullable parent, source/destination owners, revision, equipment slots |

Thus `81.0` cannot authenticate native UID81, `True` cannot authenticate integer1,
and `False` cannot authenticate owner context0. A missing parent must remain
null; a non-null parent must retain its exact unsigned native integer. Existing
`original_plan_account/posting/child/item/custody_mismatch` codes and all previous
semantic checks remain. A representation mismatch never increases
`checked.original_plans_verified`. Valid originals remain accepted, including
nested item fixtures and boundary coverage already in the broader regressions.
The audit imports no mutation codec, writer or auto-correction path.

## Native and disposable database checks

Tests ran in immutable local Linux image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with network disabled, a read-only workspace and writable `bin/`. The initial
default/mobile/source selections each freshly compiled and ran both SQL and
flatfile probe policies with `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror
-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie
-D__NO_TESTS__ -Isrc`, adding `-D__NO_MYSQL__` for flatfile. No warning suppression
or sanitizer finding occurred. Full commands and source lists are retained.

The new saved-export fixture initially used the canonical audit's streaming
cursor, causing `Previous unbuffered result was left incomplete` warnings.
The exporter expects a buffered cursor. The fixture now explicitly requests
`DictCursor` from the same SELECT-only connection and rolls back. All initial
passing runs and their exact source preimages remain retained. Final runs use
new database namespaces and contain no warning, skip or failure.

Final native probes explicitly reuse the just-built binaries only after verifying
their exact generated C++ bytes, compile commands, binary hashes and unchanged
native/migration input hashes. They run those binaries again with sanitizers.
This is recorded reuse, not a fresh-build claim or reuse of older0055 evidence.

| Variant | SQL and flatfile binary SHA-256 | Final seconds |
| --- | --- | ---: |
| Default | `dee682a63bb3b9dd916699694865e9fa086c21437cb3857b63a1d9af926b490a` | 222.614 |
| Native mobile | `e347fb89b0461cebdc3b04a8efe577c2e1fe432a58bfba98d03d8157650ab462` | 222.607 |
| Source identity | `3349ad757a9bba9299850de654b8c46cde5fb95896a9e1a85136d1c31a76658a` | 223.959 |

Each selection bootstraps isolated MariaDB10.11.14 and MySQL8.0.46 databases,
applies the real migration manifest through56, manually seeds original native
capsules and corresponding canonical rows, and proves the audit reader's UPDATE
is denied with1142. Private fixture-owner writes establish and restore fault
cuts. Audit calls use SELECT-only roles, retain rollback/cursor-close checks and
compare database inventories before/after. No live database or environment-file
credentials are used.

The original canonical SQL API/CLI tests remain:38 checks per interface for
default,38 for mobile,48 for source; total124 per interface,62 acceptances and62
refusals. Each run retains its three real schema CHECK failures per engine,
source-claim FK guards where selected, and source/intent/plan/child/custody cuts.

The new saved JSON checks damage twelve projections per engine only after the
SELECT-only export retains its original plan. Nine are equal-valued float/bool
aliases; three replace null parent with zero/float/bool. Each damaged cut runs
full reconciliation at limits0/1/100, then API and CLI exceptions/operation/
provenance views. The final three selections cover72 damaged snapshots,
216 complete reconciliation reports,648 API view checks and648 CLI checks.
Every CLI exits1 with no stderr and a global finding, even for empty filtered
views or limit0. No canonical plan or personal alias appears in output. Saved
bytes and database inventories remain unchanged.

Native capsule generation, manual SQL seeding and modeled native holdings/items
do not establish a gameplay producer, complete admitted command/receipt binding,
source capture or combined-candidate release qualification.

## Exact selections, regression and workload results

The ignored receipt runner is `tmp/plan5/run-saved-integer-checks.py`. Its mode,
complete subprocess command, explicit flags, elapsed time, stdout/stderr log,
input hashes and unchanged-source assertion are retained separately. Final runs:

```sh
PYTHONPATH=tests/async python3 -u -B -m unittest -v \
  test_reconcile_economy_accounting.ReconciliationTests \
  test_plan5_child_identity.ChildIdentityTests \
  test_economic_sql_audit_origins.ItemRevisionTests \
  test_economic_sql_audit_origins.OriginTests \
  test_economic_sql_canonical_audit.CanonicalAuditTests \
  test_economic_restore_mobile_grammar.MobilePositionTests
DURIS_RUN_AUDIT_BUDGET=1 DURIS_PLAN5_CHILD_IDENTITY_BUDGET=1 \
  DURIS_PLAN5_CHILD_IDENTITY_BUDGET_ARTIFACTS=<new-bin-namespace> \
  PYTHONPATH=tests/async python3 -u -B -m unittest -v \
  test_reconcile_economy_accounting.AuditBudgetTests \
  test_plan5_child_identity.ChildIdentityBudgetTests
DURIS_PLAN5_CANONICAL_NATIVE=1 DURIS_PLAN5_CANONICAL_ARTIFACTS=<new-bin-namespace> \
  PYTHONPATH=tests/async python3 -u -B -m unittest -v \
  test_economic_sql_canonical_audit.NativeCanonicalAuditTests
```

The native method is selected three times: default, with
`DURIS_PLAN5_CANONICAL_MOBILE=1`, and with `DURIS_PLAN5_CANONICAL_SOURCE=1`.
Final receipt modes add `DURIS_PLAN5_CANONICAL_PROBE_REUSE` only for the verified
current-source binary directories described above. Final regressions:175 pass
in82.452 seconds including input capture,74.387 in unittest. Final budgets:
three pass in28.298 seconds including capture,19.359 in unittest. These final
selections cover179 distinct methods,181 executions and zero selected skips.

The child-heavy budget retains exactly33,554,432 JSON bytes,500 modeled roots,
32,000 children and32,000 postings. CLI times at limits0/1/100 are0.854/0.781/
0.819 seconds; cumulative child peak RSS is179,644KiB, below the256MiB cap.
The separate near-limit price and mapping methods also pass their original
budgets and damaged-input/global-count checks. These are synthetic local
workloads, not release-host or service qualification.

The refreshed original mobile grammar method also passes82 native rows per
policy with exact independent agreement, including32 accepted rows. Its own
fresh build/command/input receipts remain in `grammar-current/` and adjacent
files. The primary's guarded seed integration preserves the generated probe.

## Evidence, curator handoff and remaining gates

The complete new namespace is `bin/tests/p5-saved-integer-20261005/`:
`unit-red*`, `red-observations.json`, `red-sources/`,
`red-native-comparison.json`, `initial-executed-sources/`, final executed sources,
`combined-final*`, `budget-final*`, and each initial/final native variant's
builds, capsules, exports, observations, commands and database artifacts. The
manifest hashes these artifacts, validates source/preimage pins, checks all
final counts and records the original failed/warning attempts. Previous
published manifests are referenced unchanged; no historical qualification is
promoted to this native tree.

For central registration, the primary should update the owned child pure class
from15 to18 methods, preserving all existing methods and adding:
`test_original_posting_value_requires_native_integer_representation`,
`test_original_uid_root_and_destination_owner_require_native_integer_representation`,
and `test_original_nullable_parent_requires_native_integer_representation`.
Keep canonical pure12 and its existing native
method; run default, mobile and source selections with fresh database namespaces.
No public interface, schema or wire change is requested by this fix. Consume
these five owned files and review the previously withheld broader owned import
against the completed fix; the primary alone owns that integration decision.

Full maintained SQL/flatfile builds remain blocked by the three reproduced
primary-owned omitted `native_mobile` initializers, detailed in the separately
published build handoff. Equipped baseline EAB1 evidence still has the separately
reported equipment-slot representation gap; its format/schema repair is primary
owned. Older manual exporter-fixture column/child-metadata debt remains separate
owned follow-up work. Managed restore, full retention/service budgets, authentic
producer/source capture and combined R1–R8 qualification remain open. No
executable gate is treated as passed by an inventory or isolated fixture.

This report is the curator evidence handoff for the primary's locally maintained
notebook; its locality does not block independent work. Preserve inactive
behavior, wallet-root item exclusions and the declined inactive spell change.
Accounting remains inactive; no activation, deployment, merge, production
mutation, direct experimental-accounting push or audit correction is authorized
or performed by this slice.

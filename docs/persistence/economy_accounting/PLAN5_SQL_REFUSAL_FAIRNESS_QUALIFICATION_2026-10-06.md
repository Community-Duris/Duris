# Plan 5 SQL namespace scheduling after a budget refusal — 2026-10-06

The independent aggregate SQL audit now records a scheduling refusal and persists
the next namespace after a page exceeds a resource budget. Repeated invocations
can visit roots, controls and reservations even when one page keeps refusing.
The refused namespace's complete cursor, ceiling, findings, coverage counters
and successful-page ages remain unchanged. Refusals never become canonical
corruption findings or completed coverage. All whole-audit and release flags
remain false.

## Publication and owned files

Branch `codex/accounting-plan5`; worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `bc58072b4b051ec3d103784b56993e2906eecc30`. The protected delivery receipt records the exact result SHA and
matching remote head. This commit follows the previous published slices on the
expected branch; all seven earlier branch tips and their follow-ups remain
ancestors. No rewritten history or independent experimental-accounting push.

Owned files:

- `scripts/economic_sql_canonical_audit.py`
- `tests/async/test_economic_sql_canonical_audit.py`
- `docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`
- this qualification report
- `docs/persistence/economy_accounting/PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`

The accounting schema, shared coordinator/contracts/producers, writer registry,
activation owner, native sources, migrations, shared regression/integration
manifests and runners are unchanged. Operator JSON/progress changes are owned
here; the native accounting interface does not change.

## Established defect and complete fix

Frozen base source red archive `0a908cb3672ce12537f7d30a4d70d495c05f9feda31ca0971e42d4d023553f00` contains 6,378 regular files and four
repository links. On both MariaDB 10.11.14 and MySQL 8.0.46, canonical migration
0062, the protected observer starts each namespace separately and repeats a real
SQL page twice with a one-query budget. All twelve invocations raise the existing
typed budget exception after rollback/close; the persisted scheduler remains on
the same namespace. Six observations establish starvation of siblings. Reader
accounts have SELECT grants only; no maintained data or runtime environment is
mounted. These empty-database scheduling observations establish this defect,
not immutable-evidence correctness or gameplay coverage.

The aggregate reader catches only `PageBudgetError` after successful leaf cleanup.
It rotates the scheduler and saves fixed per-namespace refusal counts/times.
It preserves all namespace states without advancing even partially examined
records. Query, byte, projection-row and cooperative time budgets use that typed
exception. Full-capture/standalone projection limits retain their previous
refusal behavior. Source/schema/type/transport/rollback/cursor-close failures
continue to propagate without saving a rotation. Root-only page mode still
raises budget refusals without advancing its checkpoint.

Progress/report formats are `economic_sql_canonical_progress_v3` and
`economic_sql_canonical_page_v3`. Progress adds exactly
`refusals.{roots,controls,reservations}.{count,last_at}`. Counts are exact integers
in `[0,2^63)`; zero iff `last_at` is null; non-null times are finite, nonnegative,
not Boolean and no later than validation time. Unknown keys/history fields
refuse. Valid v1 root checkpoints and v2 aggregate checkpoints upgrade while
preserving every existing cursor/counter/finding and v2 next namespace; old
formats have no historical refusals to import. The existing protected 32KiB
checkpoint cap, exclusive owner lock, atomic replacement and 32 retained findings
per namespace remain in force.

Reports add `page_refused`, `retained_refusal_count` and namespace
`scheduling_refusals`. Refused measurements are null, not fabricated zeroes;
consistent-page coverage is false and the backlog lower bound is zero/unknown.
Aggregate CLI status 1 persists while any refusal or finding remains, including
a later successful clean root page. All release and complete-reconstruction
claims remain false. No observation is automatically corrected.

## Exact qualified source, commands and results

Green frozen source archive SHA256 `acf901d43d877954739e9e36afc116050d270e4e0bc44ab4b05c44924d69e63c`, protected at
`D:/CodexEvidence/accounting-plan5/bin/sql-refusal-fairness-green-01-20261006/source.tar`.
Its source-transport record binds all file bytes/modes, four links, the base and
two code/test overlays. Operator/publication documents are excluded from
executable qualification. Commit delivery verifies every other source payload
and mode against this archive. Native tree `4abb609524a1f1682ea4c190f82d75003c4d679b`; migration tree
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`, canonical 0062. The primary refreshed published head is
`79df6775b92c50d9a423397e477b5e24b9e3c59d`, native tree `01291db15446d94f36e066032aa3a1eea28ef354`; it has the same migration tree but
different native inputs. Neither its private fixes nor that combined candidate
are qualified by this slice; earlier 0055/0056 cuts do not qualify these inputs.

The immutable Linux tools image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Docker runs with no network, two CPUs, 4GiB memory, 3GiB `/workspace` and 2GiB
`/tmp` tmpfs, no existing database/runtime mount, and build cache disabled.
Full launch argv, source verification helper and observer are retained in each
evidence directory. The existing private database provider creates fresh
datadirs and Unix sockets with TCP disabled; it adopts fresh bootstrap and
applies the exact owned authoritative migration chain to sequence 62.

```sh
DURIS_REGRESSION_BUILD_CACHE=off \
DURIS_PLAN5_CANONICAL_NATIVE=1 DURIS_PLAN5_CANONICAL_SOURCE=1 \
DURIS_PLAN5_CANONICAL_MOBILE=1 \
DURIS_PLAN5_CANONICAL_6972=/workspace/bin/tests/sql-refusal-canonical \
python3 -u -B tests/async/test_economic_sql_canonical_audit.py
```

Result: 67 methods, zero skips, exit 0, 178.92440790100954 seconds. This includes original
native original-plan/projection/source/mobile checks, maximum baseline bounds,
delayed lower commits, growing tails, controls/reservations and the new scheduling
regressions. Both engines retain SELECT-only denial 1142, real FK denial 1452,
inactive fixture books and complete database inventory equality around reads.
Each engine exercises all three real two-query budget refusals with unchanged
coverage state, successful rollback/close, rotation, private save/reload, and six
separate CLI processes: three one-query refusals followed by three normal pages.
The later clean root exits 1 with three sticky refusals and zero corruption
findings. Per-process exact argv, reports, query plans and inventories are in
`retained-native-tests/sql-refusal-canonical-composite/results.json`.

New pure methods protect sticky refusal fairness through durable resumes,
strict legacy upgrade and cleanup/source errors. Existing composite budget and
executor bound methods now protect rotation and paged projection-row typing.
Existing source/schema refusal, canonical findings, root-only budgets,
private checkpoint I/O, interrupted atomic replacement and all original native
cases pass. Synthetic capsules/native probes are scoped test evidence; they do
not establish genuine command receipts, writer coverage or release completion.

Native production builds were not repeated for this Python/document-only change.
All native/build/provider inputs remain exact; the canonical suite builds its
fresh original native probes. Prior production-build evidence retains its
separately recorded source scope and is not relabeled as this archive's build.

Windows command `python -B -m unittest discover -s tests/async -p
test_economic_sql_canonical_audit.py`: 67 methods, exit 0, five explicit skips
(four native methods plus the POSIX-only owner-lock case). The Linux run above
covers those skips. `git diff --check` exits 0. Normal contract validator exits
0 with 14 fixtures/920 routes/2,876 candidate sites and `release_ready=False`;
`python -B scripts/validate_economy_accounting.py --release` exits 1 with
`writer has no executable evidence`. Central regression inventory validates
920 owners; inventory validity is not runtime/release qualification.

## Narrow shared registration handoff

Primary owns changes to `tests/integration_manifest.json` and shared runners.
Its `plan5_canonical_audit_pure` entry selects only `CanonicalAuditTests`; the
new scheduler regressions require a mandatory pure selector. Register:

- `CompositeSweepTests.test_refused_roots_rotate_through_successful_siblings_and_remain_sticky`
- `CompositeSweepTests.test_legacy_all_progress_upgrade_preserves_every_namespace`
- `CompositeSweepTests.test_cleanup_or_source_refusal_never_rotates_scheduling`
- changed `CompositeSweepTests.test_composite_budget_refusal_does_not_advance_checkpoint`
- changed `CanonicalSweepTests.test_page_executor_has_query_byte_row_and_time_bounds`

Use the existing offline unittest provider with those exact argument/required-case
names, zero skips and a fresh protected test workspace. Existing default/mobile/
source entries already select `NativeCanonicalAuditTests`; add
`NativeCanonicalAuditTests.test_fair_composite_pages_and_unattached_namespaces_both_engines`
to their required cases so its actual durable/CLI refusal checks are mandatory.
No new native fields, invariant, migration or mutation consumer is requested.
Operator consumers must accept the owned v3 report fields/status semantics
documented above. These changes have not been applied to the shared files here.

## Evidence, notebook and remaining gates

All evidence is under `D:/CodexEvidence/accounting-plan5/bin/`:

- `sql-refusal-fairness-red-03-20261006`: six original defect observations, twelve attempts
- `sql-refusal-fairness-green-01-20261006`: frozen source, full run and retained native/database artifacts
- `sql-refusal-fairness-gates-01-20261006`: five local checks and exact logs
- `sql-refusal-fairness-seal-01-20261006/evidence.json`, SHA256 `248b6975153d886485459ef94f328a548cb1e1fefa897912762d8ea69d1fabfe`
- `sql-refusal-fairness-delivery-01-20261006/delivery.json`: exact result/remote SHA, owned paths, source closure and seven ancestry checks

The first red attempt failed private user-creation setup; the second used too
generous a budget on an empty control table. Both preserved attempts are excluded
from successful qualification. Final observers/source hashes, all container exit
states and every retained artifact are sealed (6972 artifacts, 4848998507 bytes).
No retained daemon remains running after completion.

This report, appended remote follow-up and protected delivery/seal constitute the
curator packet for the primary's nonblocking locally maintained notebook. No
notebook application, acknowledgement or direct cross-chat message is claimed.
Primary owns regular integration and publication of the tested combined candidate.

There is no independent slice blocker. Shared mandatory-case registration,
complete SQL/flatfile reconstruction and R7, baseline/lifecycle/orphan durable
flatfile coverage, native holdings/writer/UID coverage, genuine gameplay plus
fault/restart/replay, current combined backup/restore/retention and measured
release-host/growing-history budgets remain gates. Primary's cold-world/original
journal and private producer qualification remain separate. Maintained accounting
stays inactive; wallet-root exclusions and the declined inactive spell-path
change remain exact. No deployment, PR merge, production mutation, activation
or audit auto-correction occurs. Full Plan 5 and release are incomplete.

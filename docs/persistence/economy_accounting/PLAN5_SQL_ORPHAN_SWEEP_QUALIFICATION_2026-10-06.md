# Independent canonical orphan-ID sweep qualification

Local and remote branch: `codex/accounting-plan5`.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `ac03196f038cd8c197284f05bc0766110f31d2c3`. The solved issue/result commit and exact remote tip are
bound after publication in `resumable-orphans-delivery-01-20261006/delivery.json`.
Primary refresh `fec13b26d108b617e6b6f83b910d3f7067be20ec` adds the EBS2/ESN5/EIC2
captured-item interface handoff; its native and migration trees match this slice.
The latest checkpoint and changed shared requirements were read. Those bindings
and the private startup/world/canonical0063 successors remain unqualified here.

## Established defect and complete owned fix

The prior durable page range came only from `economic_accounting_operation`.
Retained canonical rows with a missing parent could remain unscheduled forever,
including IDs beyond the last surviving root or a store without surviving roots.
The existing whole-store reader already refuses the ordinary orphan fixtures;
its refusal did not make the bounded page reader discover the same IDs.

Three complete new pure methods fail on the frozen base reader: six nonroot
source subcases are missed, the ceiling excludes evidence beyond the last root,
and the evidence-query budget refusal is never reached (eight failure subcases).
The complete original canonical file then fails with nine subcases over 56 methods,
zero skips. Its native method stops on the first MariaDB `orphan_source_claim`
assertion: the page reports the surviving root's missing source claim, but omits
the separate rootless ID. This pre-fix run does not establish the later native
orphan-detail or MySQL cut. The red observer reaches terminal state with both
failed test results retained rather than restarted.

The SELECT-only reader now merges distinct IDs from seven existing indexes:
root, account effect, coin posting, child parent, item reference, source claim
and baseline witness. Six use `PRIMARY`; source claims use
`uq_economic_source_operation`. It captures the ceiling from the same sources,
then uses direct `operation_id > previous` single-ID seeks. A source contributes
at most page-limit-plus-one IDs; repeated details for one operation are skipped
by ID rather than counted as extra candidates. At most seven ceiling queries and
21 candidate seeks serve the two-ID page with lookahead. No aggregate or
whole-table enumeration is introduced. Invalid returned IDs refuse.

A nonzero candidate without root metadata receives a bounded root-presence
check. Absence produces `restore_economic_orphan_root_mismatch`; a present but
invalid root keeps the original metadata diagnostic. Budget/SQL failures roll
back and close the cursor without advancing the caller's progress. Routine
output remains aggregate and ID-free; bounded detailed IDs stay in protected
progress. No mutation helper is imported and no finding is corrected.

Version-1 checkpoint bytes and sticky findings retain their semantics. A saved
range finishes under its existing ceiling before the next sweep captures the
expanded range. Existing `examined_roots`, `sweep_rows` and `total_rows` now count
scheduled candidate IDs, including missing roots. Additive page fields are
`candidate_source_count` (integer seven) and `unattached_root_ids` (nonnegative
integer page count). These counts cannot be read as stored-root counts or
complete orphan coverage. Composite-key reservations and controls, complete
receipts/claims and native authority still require other scans.

Owned executable changes are `scripts/economic_sql_canonical_audit.py` and
`tests/async/test_economic_sql_canonical_audit.py`. `AUDIT_OPERATIONS.md`, this
report and the remote follow-up explain the result. No maintained C/C++ source,
migration, shared accounting contract/coordinator/producer, registry/matrix,
activation owner or central manifest is changed. No schema change is requested.

## Exact source and execution

- `python3 -B tests/async/test_economic_sql_canonical_audit.py`: exit 0, 56 methods, zero skips, 157.861035 seconds; environment `{"DURIS_PLAN5_CANONICAL_ARTIFACTS": "/workspace/bin/tests/resumable-baseline-canonical", "DURIS_PLAN5_CANONICAL_MOBILE": "1", "DURIS_PLAN5_CANONICAL_NATIVE": "1", "DURIS_PLAN5_CANONICAL_SOURCE": "1"}`; log SHA256 `acef0fe21f9025df97f060dbab858d983e67c2afd3d54d6c3d15ed44427fb211`.
- `python3 -B tests/async/test_economic_sql_audit_origins.py`: exit 0, 46 methods, zero skips, 75.644260 seconds; environment `{"DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION": "1"}`; log SHA256 `baba401b280a60636eb2c474454b14730a7765ae48e3f5535dd6824b986307ee`.
- `python3 -B tests/async/test_restore_economic_coin_effects.py`: exit 0, 1 methods, zero skips, 278.177595 seconds; environment `{"DURIS_PLAN5_CANONICAL_EVIDENCE": "1", "DURIS_RUN_RESTORE_COIN_INTEGRATION": "1"}`; log SHA256 `bc1ade63865cad82f374cee6a47627921d7b9a514728ed6cedb69b16c322b9d9`.

All **103 methods pass, zero Linux skips**. Frozen archive SHA256
`052712aff313e5adcf8a44da3264d7d6cfb806702e3703e1c941a5dac6bb0e2d` contains 6369 regular files,
original Git modes and four repository links with three owned overlays. Every
regular file is hashed before/after execution. Delivery rechecks all tested
bytes/modes; publication documentation is hashed separately and excluded from
executable qualification. Native tree `4abb609524a1f1682ea4c190f82d75003c4d679b` and migration
tree `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` are unchanged.

Pinned image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`
uses GCC13.3/Python3.12, two CPUs/4GiB, no network/ports and private RAM work/tmp
trees. Original C++20/Werror/O1/debug/ASan/UBSan/non-PIE/crypto recipes build fresh
probes without object/probe reuse. No maintained data or environment file is
mounted. The private root remains writable for the unchanged full coin/restore
fixture's temporary directory under `/`. Original providers, flags and deadlines
remain intact; source freeze and observation do not extend an individual test's
deadline. No maintained C++ build is claimed for this Python-only change.

Fresh MariaDB10.11.14 and MySQL8.0.46 apply all canonical0062 receipts. Both
engines detect the original rootless detail and source-claim fixtures and retain
their restored controls: four rootless page observations, each with one missing
root among two candidate IDs. The original FK denial is still checked; imported
corruption is modeled only inside the disposable fixture under its existing
temporary FK-disable procedure, followed by its original restoration. This is
not evidence of a legal producer creating an orphan under normal constraints.

All 88 original canonical cuts retain page SELECT statements, exact rollback/
cursor-close counts of one and unchanged original application inventories.
The two intact pages retain 28 EXPLAIN plans for actual ceiling and first range
seeks over all seven sources. Populated sources must use their expected index;
empty-source plans are retained but are not populated-index proof. The pure
fixture also proves deduplication of 10,000 details for one ID, durable save/load,
a late lower ID discovered on the following sweep, sticky findings and budget
refusal. All three new regression method ASTs are identical between the red and
successor freezes; only the pure helper and native evidence collector needed
adaptation. That modeled cardinality is not a release-host performance result.

The packet retains 186 measured pages: 88 canonical
cuts, 18 moving-view/delayed-commit observations, four modeled maximum-reservation
cuts, 64 original native baseline pages and 12 book-refusal pages. Another 12
restored-book pages execute and pass, but their temporary metric files are removed
by the unchanged original fixture. Retained maxima are
110 queries, 6379015 bytes and
0.301388827s, within the existing component bounds. The
9,071-reservation maximum is modeled. The authentic native baseline fixture is
four witnesses over two books per engine. Original 20 numeric alias refusals,
20 restored alias controls and three native book refusals/restored controls per
engine remain passing. No 1,000-root workload, p95/p99, retained-storage growth,
whole-history fairness or release-host budget is qualified.

Windows runs 53 selected methods with the original POSIX-lock skip. Linux
executes the original lock/SIGKILL case with zero skips. Normal accounting
validation and whitespace pass. `--release` still exits1 with
`writer has no executable evidence`. The full native coin/restore fixture's
private synthetic activation remains disclosed and does not qualify genuine
R6 or authorize maintained activation.

## Narrow primary-owned registration and consumer handoff

In `tests/integration_manifest.json`, retain all previous cases and the request
that `plan5_canonical_audit_pure` selects `CanonicalSweepTests`. Append:

- `CanonicalSweepTests.test_orphan_candidates_from_each_indexed_source_are_not_hidden`
- `CanonicalSweepTests.test_candidate_sweep_merges_duplicates_and_resumes_delayed_lower_ids`
- `CanonicalSweepTests.test_candidate_budget_refusal_does_not_advance_progress`

The original native method names/arguments remain; its two existing orphan cuts
now assert durable page findings. Preserve variants, environments, original
providers, fresh namespaces and original deadlines. Consumers are central
passed-case validation and operator/qualification reports. Any report consumer
must interpret row counts as scheduled candidate IDs and keep coverage false;
the two additive page fields above require no checkpoint/schema change. No
registration application, primary acknowledgement or variant waiver is claimed.
The primary validates its actual integrated candidate.

## Protected evidence and outstanding gates

Protected root `D:/CodexEvidence/accounting-plan5/bin/` retains
`resumable-orphans-red-01-20261006`, both `resumable-orphans-green-01-20261006`
and `resumable-orphans-green-02-20261006`, and both component-gate folders.
Expected red command exits1 and final green commands exit0 are separately
authenticated. All original observing containers finish with no OOM or restart.
Observation expiration never restarts a process.

The first fixed-source attempt fails its new index-plan collector: it matched
direct SELECT text although the original executor wraps SELECTs in a bounded
subquery. That run executes 56 methods with one failed assertion, zero skips,
and stops before origin/restore commands. Its exact frozen source, logs, terminal
state and original helper remain preserved. The collector was corrected to match
the actual bounded statement; the successor is freshly frozen and built under
the original deadlines. No production reader or existing oracle was relaxed,
and no positive result is attributed to the failed source.

A local new-test attempt had one fixture failure because its root-presence stub
also intercepted the joined lifecycle query. The stub was narrowed before the
fixed-source freeze. Its summary/tool chunk is retained in
`resumable-orphans-local-fixture-observation-01-20261006`; the complete original
tool output is not claimed copied. It is not a production defect or a native
restart. Seal `resumable-orphans-seal-01-20261006/evidence.json`, SHA256
`1dd1fc86a2c6346938de9431b21f58c09dba1275082e24e9efa9062a099d697b`, inventories 9245 regular artifacts,
5492433562 bytes and zero links using streaming no-follow hashes.
The delivery receipt binds source/result/remote, owned paths and all seven earlier
ancestor tips. Logs, binaries, archives, data and scratch helpers are uncommitted.

An early seal attempt ran after the tests passed but before the same original
helper finished harvesting its artifacts. It refused the missing native-build
file and created no successful seal. The exact failed helper and summary/tool
chunk remain in `resumable-orphans-seal-harvest-observation-01-20261006`; the
complete original tool output is not claimed copied. The original process was
observed through closure before the successful seal; no test was restarted.

Whole-store control/reservation/orphan enumeration, pending-claim consumption,
complete receipts/current native holdings, fair durable flatfile scans and a
full quiescent comparison remain open. EBS2/ESN5/EIC2 independent reconstruction,
genuine source capture/borrowed activation verification, original missing recovery
source claims, real producer/publication/ACK/lost-reply/cold-world journeys,
typed active erasure, managed backup/restore/retention and full release budgets
remain. Independent Plan1 acceptance stays completed at its recorded scope.
Wallet-root exclusions and the declined inactive spell path are preserved.
Full Plan5/R7/R8/release remain incomplete; no genuine blocker prevents further
owned reader work.

The primary-local notebook remains nonblocking. This owned report/seal/delivery
packet supplies its curator workflow without a cross-chat message or claimed
acknowledgement. Maintained accounting stays inactive; no production operation,
activation, deployment, PR merge or push to experimental-accounting occurs.

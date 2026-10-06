# Current native stake and audit-budget qualification

Branch/worktree remain `codex/accounting-plan5` and
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Exact tested source and report base are `19af82ffbc75212b4ff14cf4f4d70aca71dbe628`.
The primary was refreshed to `4ad525878f30e3aad2f1bbd86840a8ef31b86511`
before this qualification. This slice owns only this report and the remote
followup. It changes no implementation, test, shared interface, schema,
producer, coordinator, registry/matrix or activation owner.

## Previously skipped methods executed

The [combined reader qualification](PLAN5_PRIMARY_PAIR_COMBINED_QUALIFICATION_2026-10-06.md)
passed 213 executed methods and retained three original opt-in skips. Its
observer's incorrect zero-skip expectation and exit 1 remain preserved.
Those three complete original methods are now executed on the same current
code inputs with their native/budget flags enabled. They all pass with zero
skips. No original test, decorator, assertion, provider, compiler flag,
deadline or measured acceptance bound is modified.

- `python3 -u -B -m unittest -v test_reconcile_economy_accounting.NativeStakeSQLTests` — exit 0, 1 method(s), zero skips, 93.367959 seconds. Environment: `{"DURIS_PLAN5_PRICE_AUDIT_ARTIFACTS": "/workspace/bin/tests/plan5-price-view-sql/current-optin-20261006", "DURIS_RUN_STAKE_SQL_INTEGRATION": "1"}`. Log SHA256 `7a34ee7cead4b71af87fcb2e7b17c500ff2c39556adab9e348303444ac788ed8`.
- `python3 -u -B -m unittest -v test_reconcile_economy_accounting.AuditBudgetTests` — exit 0, 2 method(s), zero skips, 12.971578 seconds. Environment: `{"DURIS_RUN_AUDIT_BUDGET": "1"}`. Log SHA256 `5d638060315db987f74e10b9260047778749f667c4d877b6bd0ea85a14f26161`.

The native method compiles both SQL and flatfile fixtures from the original
C++20, Werror, O1/debug, ASan/UBSan, non-PIE and crypto recipe. It directly
invokes the compiler; no prior objects or build cache are reused. Both emitted
fixtures retain SHA256
`1ed10a94436e14271b6bc07d8f17ebf364b10dcd770e1da1c6738df1d02c7281`.
The original native controls cover 104 source-grammar cases, 1,107 source-policy
decisions, six original-operation-link decisions and four price roots in two
epochs. Native encoder/independent reader agreement is retained.

Fresh MariaDB 10.11.14 and MySQL 8.0.46 identify canonical 0062/all 62 receipts.
The method completes 90 read-only stake captures, eight price captures,
24 price CLI cases, count/result/index alias refusals, eight index-density
captures, source-kind/original-self refusals and 16 price-projection scope cuts.
Both SELECT-only users refuse UPDATE; all seven source tables stay unchanged
during reads and deliberate fixture changes are restored. These are native
structural encodings and modeled SQL cuts, not real gameplay or producer proof.
Accounting stays inactive in these fixtures.

The two full budget methods retain 12 fresh CLI measurements, clean/corrupt
controls and detail limits 0/1/100. All exception/count results and original
input bytes stay invariant. The price fixture has 100,000 captured roots and
33,552,384 bytes including its original valid JSON whitespace; the mapping
fixture has 9,948 roots and 33,551,795 structured bytes. The mapping method
also retains both over-limit refusals at 33,554,433 bytes. Each measured CLI
keeps its original 30-second and 256-MiB assertions and 35-second process limit.
Observed maximum is 0.985723204 seconds and
144097280 bytes. These are synthetic component budgets;
they do not qualify the release host, mixed native 1000-root workloads,
game-loop p95/p99, database query plans or storage/checkpoint growth.

## Exact source and protected evidence

Archive SHA256 is `db3bccaabc5a1cf6d5323e738109d229097a7f98b63c24d74a48ecbb05ae54c1`.
This is a raw Git archive with no overlays: all 6,365 regular source files,
their original archive modes and four repository links are recorded. The
loader verifies source bytes before and after the original methods. The native,
migration, script and test trees equal the earlier tested code
`8a9a6f0cf72fbe25f6daeb102924f3e949e77134`; only publication documentation differs.
Together the two qualifications cover 216 distinct executed methods, but the
earlier whole-file skips and failed observer are not relabeled or erased.

Native tree is `4abb609524a1f1682ea4c190f82d75003c4d679b` and migration tree is
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`. The pinned private image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC 13.3/Python 3.12, two CPUs/4 GiB, read-only root and private workspace/tmp
RAM mounts. No network, host port, maintained data, environment file or
production database is mounted. Original observer exits 0 in
106.428507 seconds under its 7200-second observation bound;
the same original container exits once with no restart or OOM.

Protected root: `D:/CodexEvidence/accounting-plan5/bin/`.
Execution is `current-optin-qualification-01-20261006/`; seal is
`current-optin-qualification-seal-01-20261006/evidence.json`, SHA256
`7fb1cad4a12b9d50f183880700bb6671e627a0060f1fa504f3a3fb48f4037baf`. It inventories 81 regular artifacts,
294583489 bytes and zero links without following links.
Frozen sources, executed helpers, complete original logs and both native
binaries are retained outside Git. No fresh maintained build is claimed:
C/C++ is unchanged and the prior production build evidence retains its scope.

## Remaining full acceptance

The current [Plan5 contract](plans/05_AUDIT_OPERATIONS_AND_RELEASE.md) and
[#490 sections 4, 6 and 8](https://github.com/Community-Duris/Duris/issues/490)
still require the complete independent audit, durable resumable fair sweeps,
protected views/reward projection, lifecycle/retention, verified restores,
supported player journeys and full release budgets. Current canonical capture
is one RR read-only transaction bounded by 100,000 roots and 32 MiB; its CLI
has no durable sweep/progress interface. IDs/timestamps cannot be treated as
commit watermarks, and interrupted/moving/incomplete scans cannot be an all-clear.
This is an unresolved owned behavior, not completed by these budget fixtures.

The two missing producer source claims, complete source/custody capture,
borrowed-session activation verifier, shared player-inspector provider request,
actual producer/publication/ACK/lost-reply/cold-world proof, typed active erasure,
full flatfile parity, managed backup/restore/retention and release-host numeric
workloads remain open. The primary's private schema 0063/birth evidence is not
qualified by this canonical 0062 component source. Missing executable writer
evidence continues to block release; no Plan or R1–R8 completion is claimed.

Maintained inactive behavior, wallet-root exclusions and the declined inactive
spell change remain. No production operation, accounting activation, deployment,
PR merge or audit correction occurs. This report and protected seal are the
curator packet on the expected remote branch; the primary's local notebook
remains nonblocking as directed. Application/acknowledgement is not claimed.

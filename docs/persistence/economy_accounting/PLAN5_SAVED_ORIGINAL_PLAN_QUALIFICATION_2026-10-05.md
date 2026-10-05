# Plan 5 saved original-plan projection qualification

The saved reader previously accepted coherent rewrites of child derivations,
posting children and item children despite disagreement with the retained
native plan. This slice requires original EAP1 evidence for every committed
root and independently compares its projections. All selected Plan 5 checks
pass; the two primary-owned source-contract failures and release refusal remain
open. This is component qualification, not release or activation acceptance.

## Source, branch and ownership

- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Delivery branch: `codex/accounting-plan5`, as required by the user and expected
  by the primary. Its existing tip
  `50495fdd2fdf43ef5c513e6eb95f543a886b2711` is preserved through ordinary merges.
- Refreshed primary: `39ad28e99348ad738ace39f6414a160ce3196ceb`.
- Preserving merge/base: `51b9e05c5ae6f2f976ac31dd7e92ef09f1786aba`.
- Native tree: `255d78c159d68bb4516b493ec38d477832b53c43`.
- Migrations tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`.
- Canonical schema: fresh bootstrap through `0056_spell_ward_durability`.
- Result SHA and remote-tip verification: delivery receipt
  `tmp/plan5/saved-plans-delivery.json` and the delivery message.

Owned files are the independent reconciler, SQL audit exporter, their existing
reconciliation/child test modules, `AUDIT_OPERATIONS.md`, and this report.
No shared native source, contract, coordinator, producer, migration, registry,
matrix or activation-owner file is edited independently. The tested base imports
the primary's two additional SHOP commits. The completed slice, earlier canonical
SQL command and native/recovery qualification reports are consolidated onto the
delivery branch without discarding its history. Historical reports retain their
original source pins; their results do not qualify the newer combined source.
The other Plan 5 branches and evidence remain available.

The source freeze records 6,152 regular tracked inputs and all 1,511 native/
migration inputs, matching Git blobs. All 6,147 unowned tracked inputs are
preserved. Final receipts record source SHA-256 before/after each selected run.
Key final source hashes are:

| Input | SHA-256 |
| --- | --- |
| `scripts/reconcile_economy_accounting.py` | `a745f1851c9aad64873d0a1f2a79a5d74e902749f09a7d60506316300181f334` |
| `scripts/economic_sql_audit_snapshot.py` | `9cd29eacf5ca637e1e13f567bca8a9de3af5c695bba6ccf66fe9a1724b924c50` |
| Existing independent EAP1 decoder | `dcf870dbf4abf5c2dc762cdab0ac73468945e590d0ba51e3f8c6d3315a60f5cb` |
| `tests/async/test_reconcile_economy_accounting.py` | `f2d75fabd3c1a174fe17f569b3aecff41d3c9f3ae97733edacde25cedbba3928` |
| `tests/async/test_plan5_child_identity.py` | `9289633d8e80bd7292e3c246140ae2d09095e672bd72afca8eb0575b1c6982e2` |

## Defect and complete reader fix

The preimage reports zero exceptions for three native-backed cuts: replace
the first child's domain and consistently rederive both IDs; move the first
posting to child zero; or move the item reference to child zero. All root
counts, balance totals and ordinary links still agree. The retained EAP1 does
not agree. Corrected BEFORE custody facts are taken from that original plan;
the initial modeled BEFORE mismatch and failed diagnostic are retained rather
than being treated as a valid intact native cut.

The reader now requires lowercase original plan hex and persisted plan/intent/
domain digests for every committed root, independent of backend labels. It
decodes EAP1 with the existing independent Python decoder, then checks original
root metadata, six counts and three digests. Foreign or unbound capsules do not
authenticate this root's details. It compares all account effects, posting
event/account/child links, complete child derivation facts, item event/child/
UID/revision links, and linked native BEFORE/AFTER custody facts and equipment
slots. Exact integer checks prevent Boolean equality from supplying proof.
Rejected roots cannot carry a plan. Missing proof is an exception; no optional
attestation marker or inferred plan is accepted.

The SQL exporter retains the existing metadata and EAP1 columns and the omitted
posting event, item-reference line and ledger BEFORE/equipment facts in the
same read view. It preflights root count, maximum plan size and aggregate
hex-expanded size before fetching plans. Each plan is at most 4 MiB; the whole
JSON cut remains at most 32 MiB. Operator views omit canonical bytes and aliases,
preserve global findings even for unrelated filters or limit zero, and never
change authority or the input. Opaque EAI1 facts are not exported.

Synthetic positives now explicitly author their original plan before damage.
Native positives use actual C++-encoded EAI1/EAP1 bytes. Existing fault tests
retain their original findings and also expect the new original-plan findings;
neither tests nor native structural checks are weakened.

## Runtime and commands

All executable checks use immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
Ubuntu 24.04.4, GCC 13.3 and Python 3.12.3. The repository is mounted read-only;
only `bin/` is writable for test artifacts, with no container network. Native
database checks additionally use `SYS_ADMIN` and unconfined seccomp/AppArmor
for the existing private database namespaces. No local environment credentials,
production data, gameplay activation or operational correction is used.

Set `PYTHONPATH=/workspace/tests/async` and
`PYTHONDONTWRITEBYTECODE=1`. Commands below are exact selected test commands;
wrapper receipts also retain environment, elapsed time and source hashes.

| Command | Result |
| --- | --- |
| `python3 -u -B -m unittest -v test_reconcile_economy_accounting.ReconciliationTests test_plan5_child_identity.ChildIdentityTests` | 129 methods pass, zero skips |
| `python3 -u -B -m unittest -v test_economic_sql_audit_origins.ItemRevisionTests test_economic_sql_audit_origins.OriginTests` | 25 methods pass, zero skips |
| `python3 -u -B -m unittest -v test_plan5_child_identity.NativeChildIdentityTests` | One method passes, both native configurations and SQL engines mandatory |
| `python3 -u -B -m unittest -v test_reconcile_economy_accounting.NativeStakeSQLTests` | One method passes, complete existing both-engine matrix |
| `python3 -u -B -m unittest -v test_plan5_child_identity.ChildIdentityBudgetTests` | One method passes, original 30-second/256-MiB budgets |
| `python3 -u -B -m unittest -v test_reconcile_economy_accounting.AuditBudgetTests` | Two methods pass, original budgets |
| `python3 -u -B -m unittest -v test_economy_writer_coverage_contract test_audit_accounting_invariants` | 71 methods executed: 69 pass, one failure, one error, zero skips |
| `python3 scripts/validate_economy_accounting.py` | Exit 0; `release_ready=False` |
| `python3 scripts/generate_economy_writer_coverage.py --check` | Exit 0 |
| `python3 tests/run_integration_matrix.py --list` | Exit 0; inventory only |
| `python3 scripts/validate_economy_accounting.py --release` | Exit 1: writer has no executable evidence |

Native child selection sets `DURIS_PLAN5_CHILD_IDENTITY_NATIVE=1` and
`DURIS_PLAN5_CHILD_IDENTITY_ARTIFACTS=/workspace/bin/tests/plan5-saved-plans-2026-10-05/native-final`.
Native collector selection sets `DURIS_RUN_STAKE_SQL_INTEGRATION=1` and
`DURIS_PLAN5_PRICE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-price-view-sql/saved-plans-collector-2`.
Child budgets set `DURIS_PLAN5_CHILD_IDENTITY_BUDGET=1` and artifacts to this
slice's `child-budget-1`; the two audit budgets set `DURIS_RUN_AUDIT_BUDGET=1`.

## Native, SQL and budget evidence

Both fresh native probe builds use C++20, `-Wall -Wextra -Wpedantic -Werror`,
ASan/UBSan, `-O1 -g`, `-fno-omit-frame-pointer -fno-pie -no-pie`,
`-D__NO_TESTS__`, `-Isrc` and `-lcrypto`; flatfile additionally defines
`__NO_MYSQL__`. Exact compiler source lists and argv are retained in
`native-final/native-builds.json`. Both builds/runs exit zero with no probe
stderr and no reused binary. The generated probe SHA-256 is
`8f0b971b09800cbffe7e5e70185950010a98842ec288d6db47c5f90a26f41173`;
both final binary hashes are
`10c8824122c6ec648912dfe2a38cf5d2f8974e435c5e2474204f4c0be90cca7a`.
Fourteen existing native child-contract cases are retained. Actual encoded
plans contain two accounts, two postings, two children and a player8-to-player7
transition for UID81, with revision1-to-revision2.

Fresh MariaDB `10.11.14-MariaDB-0ubuntu0.24.04.1` and MySQL
`8.0.46-0ubuntu0.22.04.4` schemas both end at canonical0056 and remain inactive.
Each grants the auditor SELECT only and rejects UPDATE with1142. The child
test records30 read-only captures,20 damage/projection cuts and96 CLI checks.
The six new both-engine cuts reproduce the three old false cleans and now
produce exactly their original-plan child/posting/item mismatch. Owner repairs
of disposable fixtures restore the original rows; audits never repair them.
Native UNIQUE child constraints remain installed and refuse duplication.

The retained full collector matrix additionally executes90 read-only captures,
72 fault captures,62 source-fault captures,44 source-kind cuts,104 native
source-grammar cases,1107 native source-policy decisions, six native original-
link decisions, eight SQL density cuts, four price roots across two epochs,
24 price CLI checks and16 price-scope conflicts. Its unchanged native metadata
and structural decisions retain their full counts. Correct accepted model
original links now have corresponding original plan bytes; post-commit damage
keeps the original capsule and adds the precise plan finding.

The child budget uses500 model roots,32000 children and32000 postings, with
original plans authored before capture:18,121,727 unpadded bytes and exactly
33,554,432 input bytes. Limits0/1/100 pass in0.715/0.732/0.667 seconds;
cumulative child peak RSS is161240 KiB, below262144 KiB. These are synthetic
reader measurements, not release-host or real producer qualification. The
mapping and price budgets also retain their byte/row limits and original
latency/memory ceilings; their detailed results remain in `budget-final.log`.

Evidence root: `bin/tests/plan5-saved-plans-2026-10-05`. It contains preimages,
red native-backed cuts, green read-only reproductions, all attempted logs,
final command/source receipts, native source/binaries, SQL captures and all
budget outputs. The collector's fresh namespace is retained separately at the
path above. `tmp/plan5/saved-plans-evidence.json` seals these artifacts and the
68,183 prior artifacts; `saved-plans-delivery.json` verifies committed owned
bytes and the remote tip. Earlier unit failures, the first native fixture's
overwritten modeled-origin failure, and the collector's stale modeled-original
failure are retained with their exact failed results.

## Narrow primary handoff and remaining gates

No SQL schema or native interface change is requested. Snapshot producers and
retained flatfile collectors must supply the original plan/metadata/digests and
posting/item/ledger fields documented above from retained authority. Consumers
are the independent reconciler and existing ID-only views. They must not derive
or reseal original plans from projections. Original fields remain immutable;
missing retained evidence stays an exception. Complete original EAI1 fact,
CCM1 accepted-time binding and receipt-payload authentication remain open.

The existing central `plan5_child_identity_pure` row still reports required8,
while the selected `ChildIdentityTests` class now executes15 methods. The
primary owns any update of this row to15 and related source inventory; retain
its existing class/engine ownership and unqualified writer status. No new
dependency, classification waiver or writer promotion is requested.

The two shared source contracts still fail on the exact frozen primary source:
the checked SHOP placement probe searches the delegating wrapper instead of
`shop_trade_publish_physical_impl`; the SQL route probe still expects old
line887 instead of the current owned SQL sites. Their prior narrow handoff
remains applicable. Preserve complete checked-site coverage and exact owners.
The normal validator/matrix/inventory passes do not waive these failures.

No maintained server build or actual player journey is claimed for this Python
slice. The previous candidate's SQL compile failure is not a new build result
for native tree255d78c; the same primary-owned `intent.epoch` source expression
remains present and needs the existing `intent.admission.metadata.epoch` owner
repair and current combined native qualification. The retained-publication
fixture include failure and SQL managed service/recovery gates also remain.

R1–R8 still need complete producer/authority/receipt integration, every supported
money/item writer, explicit active refusal for unsupported routes, real gameplay
and restart/lost-reply journeys, complete admitted holding/UID enumeration,
both-backend fresh/upgrade/retention/service evidence and release-host operation,
latency, growth, checkpoint and replica budgets. No inventory count, model
fixture, original-plan pass or old source's build substitutes for this proof.
Accounting remains inactive; wallet-root exclusions and the declined inactive
spell change are preserved. No production mutation, activation, deployment,
PR merge or independent push to `experimental-accounting` occurs.

This report is the curator handoff for the primary's locally maintained shared
notebook, as directed by the user. Notebook locality is not a blocker. The
primary integrates the solved slice and publishes the tested combined candidate.

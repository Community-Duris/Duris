# Plan 5 independent native-mobile custody grammar qualification

The native plan and baseline codecs accept original native-mobile owner12.
Both independent readers still stopped at owner11, rejecting16 native-accepted
EAP1/EAB1 cuts. This slice supports that original identity and its complete native
equipment/state bounds without importing native mutation logic or changing an
authority, producer, schema or activation path. Full release remains incomplete.

## Source, delivery and ownership

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `fbe37541dcf753aea5eb98dc093e7d95daa8382b`.
- Refreshed primary consumed: `1e36e8b06b51e30dc72e1d99824c1c4e425b2d0f`.
- Native tree: `d9fc5c96696a71904d2e18c5edf15cd48f0d9423`.
- Migrations tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, canonical0056.
- Result SHA/remote verification: `tmp/plan5/mobile-grammar-delivery.json`.

Owned files are `scripts/economic_restore_evidence.py`,
`scripts/economic_sql_audit_origins.py`, the new
`tests/async/test_economic_restore_mobile_grammar.py`, the existing canonical
SQL audit test, `AUDIT_OPERATIONS.md` and this report. Shared native/coordinator,
contracts, producers, migrations, registry/matrix and activation files are not
independently edited. No persisted field or native interface change is requested.

The independent EAP1 reader accepts owner12 only with a nonzero original lifetime
ID below `UINT64_MAX` and context0. Its equipped items require slot<=43, active
state, no parent and root==UID; its historical player rules retain their prior
slot range. The EAB1 reader applies the same identity/context and accepts mobile
active/quarantined states. A destruction witness cannot retain mobile ownership.
No personal alias or runtime/VNUM identity is inferred.

## Defect and complete verification

Fresh strict native probes author/decode both EAP1 and EAB1 with the actual
current C++ codecs, then independently compare82 payload decisions per backend:
64 BEFORE/AFTER plan cuts and18 original witness cuts. Native accepts32 and
refuses50. The original readers reject16 accepted mobile cases. The first
correction also admitted one native-refused destruction witness; the final state
bound fixes that refusal. All failed commands, capsules and exact mismatch lists
remain preserved. No native grammar check or old owner refusal is weakened.

The final independent readers agree on all82 cases in both SQL and flatfile
configurations. Both generated probes compile/run with strict C++20 ASan/UBSan,
no binary reuse, and binary SHA-256
`887085e32a05885ac7001b9840220a67fb25548ea3be7d9a0cb6077e1192411c`.
Native source lists/flags are in `native-green/native-builds.json`.

The canonical SQL fixture now derives its BEFORE/AFTER ledger facts and fixture
repairs from the retained original plan. The mobile variant uses actual owner12,
ID42, context0 and BEFORE equipment43, transferring to player7 with slot0. The
original player8-to-player7 variant remains. Earlier fixture failures from a
shadowed inventory variable and a player-specific repair value are retained.
Audit transactions themselves always roll back and never correct findings.

All checks use immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
Ubuntu24.04.4/GCC13.3/Python3.12.3. The repository is read-only, only private
`bin/` artifacts are writable, and there is no network. Native database checks
use the existing private namespace privileges. Set
`PYTHONPATH=/workspace/tests/async` and `PYTHONDONTWRITEBYTECODE=1`.

| Exact selected command | Result |
| --- | --- |
| `python3 -u -B -m unittest -v test_reconcile_economy_accounting.ReconciliationTests test_plan5_child_identity.ChildIdentityTests test_economic_sql_audit_origins.ItemRevisionTests test_economic_sql_audit_origins.OriginTests test_economic_sql_canonical_audit.CanonicalAuditTests test_economic_restore_mobile_grammar.MobilePositionTests` |170 methods pass, zero skips |
| `python3 -u -B -m unittest -v test_economic_restore_mobile_grammar.NativeMobileGrammarTests` |One method passes, both configurations and all82 decisions mandatory |
| `python3 -u -B -m unittest -v test_economic_sql_canonical_audit.NativeCanonicalAuditTests` |Passes separately for original player and equipped mobile variants, both engines mandatory |
| `python3 -u -B -m unittest -v test_reconcile_economy_accounting.AuditBudgetTests test_plan5_child_identity.ChildIdentityBudgetTests` |Three methods pass, original byte/latency/memory limits |

Native grammar sets `DURIS_PLAN5_MOBILE_GRAMMAR_NATIVE=1` and
`DURIS_PLAN5_MOBILE_GRAMMAR_ARTIFACTS=/workspace/bin/tests/plan5-mobile-grammar-2026-10-05/native-green`.
SQL selections set `DURIS_PLAN5_CANONICAL_NATIVE=1`, with
`DURIS_PLAN5_CANONICAL_ARTIFACTS` ending in `native-sql-player-final` or
`native-sql-mobile-final`; the latter additionally sets
`DURIS_PLAN5_CANONICAL_MOBILE=1`. Both strict probe configurations are freshly
built for each variant. Actual MariaDB10.11.14 and MySQL8.0.46 each apply
canonical0056 in four fresh private schemas across the variants. There are76
read-only API captures and76 CLI checks:40 accepted intact/repaired cuts and36
refused cuts. All inventories are unchanged, every reader rolls back, all four
SELECT-only roles reject UPDATE, and12 schema constraint checks refuse damage.
Twelve damaged saved projections retain the precise original-plan findings.
This is175 distinct selected methods,176 executions because the same native SQL
method is selected with two fixture variants. There are zero skips.

Budget selection sets `DURIS_RUN_AUDIT_BUDGET=1`,
`DURIS_PLAN5_CHILD_IDENTITY_BUDGET=1` and
`DURIS_PLAN5_CHILD_IDENTITY_BUDGET_ARTIFACTS=/workspace/bin/tests/plan5-mobile-grammar-2026-10-05/budget-final-children`.
The exact32-MiB child cut contains500 modeled roots,32000 children and32000
postings. Limits0/1/100 pass in1.102/1.260/1.404 seconds, with cumulative peak
RSS179560 KiB below262144 KiB. Mapping and price tests retain their near32-MiB
inputs and original30-second/256-MiB limits. These are synthetic measurements,
not release-host workload qualification.

| Final input | SHA-256 |
| --- | --- |
| Independent EAP1 decoder | `8dc9aa2e4c1f7a08178854b3410f054a31c696a94765a9b251526d0b13ef4613` |
| Independent EAB1/origin decoder | `9d1a5f03950c61b8794eb066c1456fd0ce1d434b1d05cbc4929f9baf0844d620` |
| New grammar test | `82c046a6c1e50abc170b4fdf4b37bf2fabf538839dbef119c4894a0387836d6b` |
| Parameterized canonical SQL test | `bddb8d5d8b276b6442045c7873d8908253a270c6f0e3f4b751bfd280917a1f95` |

## Evidence, handoff and remaining gates

Evidence root: `bin/tests/plan5-mobile-grammar-2026-10-05`. It retains source
preimages, every failed/passing attempt, original native bytes, generated source,
fresh binary/command/source receipts, private SQL results and budget snapshots.
`tmp/plan5/mobile-grammar-evidence.json` seals new artifacts and inherits the
previous sealed evidence index without claiming a new full old-archive rehash.
`mobile-grammar-delivery.json` verifies committed owned bytes and the remote tip.

The primary owns registration of the new `MobilePositionTests` class (six pure
methods), `NativeMobileGrammarTests` (one method, both configurations mandatory),
and the existing canonical native class's mobile selection with the explicit
flag, fresh artifact namespace, both-engine markers and zero-skip enforcement.
Retain unqualified writer status; no producer coverage promotion is requested.
The older child count8-to15 request remains. Consumers are the independent
restore verifier, baseline origin exporter, canonical SQL operator and saved
reconciler. Original owner12 IDs/context/equipment must come from retained native
evidence, never a resealed projection or inferred current runtime identity.

EAB1 has no equipment field. The witness cases here carry slot0; complete
equipped baseline/recovery requires its original EAP1 and retained native
boundary and is not established by this grammar agreement. Actual quest/item
producer integration, complete native census, baseline admission-time/receipt
authentication, both-backend gameplay/restart/lost-reply and retained restore/
retention/service journeys, and release-host operation/growth budgets remain.
The previously reported shared contract/build/publication gates are not newly
executed or waived here. Accounting remains inactive; wallet-root exclusions and
the declined inactive spell change remain. No production mutation, activation,
deployment, PR merge or experimental-accounting push occurs. This is the curator
handoff for the primary's locally maintained notebook, whose locality is not a
blocker. The primary integrates completed slices and qualifies the combined release.

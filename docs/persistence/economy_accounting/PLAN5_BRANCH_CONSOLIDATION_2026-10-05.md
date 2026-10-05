# Plan 5 branch consolidation and combined operator qualification

All completed Plan 5 slices are delivered through `codex/accounting-plan5`,
the remote branch required by the user and expected by the primary. Ordinary
merges preserve its existing history and the other branches. The primary's
native source and canonical0056 migration trees remain unchanged. Full release
qualification is still incomplete.

## Branch, source and preserved slices

- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Original delivery tip: `50495fdd2fdf43ef5c513e6eb95f543a886b2711`.
- Tested saved-plan base: `51b9e05c5ae6f2f976ac31dd7e92ef09f1786aba`.
- Refreshed primary consumed by that base:
  `39ad28e99348ad738ace39f6414a160ce3196ceb`.
- Native tree: `255d78c159d68bb4516b493ec38d477832b53c43`.
- Migration tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`.
- Consolidation base: `b48824523c0b5ab5926d0a724f9e3c670492be82`.
- Final result and verified remote tip: `tmp/plan5/saved-plans-delivery.json`.

| Preserved slice | Original result | Current follow-up |
| --- | --- | --- |
| Original child identity audit on delivery branch | `50495fdd2fdf43ef5c513e6eb95f543a886b2711` | Preserved ancestor; updated saved reader/native fixtures qualified |
| Published0056 native and managed recovery qualification | `32d829ef2c3010e523e2afac8772c0c56ef9c5b8` | Qualification report restored on delivery branch with its historical source pin |
| Canonical SQL operator command | `36355900e9cdf28e83413b5171cce2d43a3b9864` | Command, tests and report merged; unit and both-engine native checks rerun on current source |
| Coherent0056 build/recovery qualification | `42141f6268707787b44d3b95bab2290ae3898b87` | Report preserved; earlier build results remain tied to that source |
| Saved original-plan projection fix | `be854d0dfab3b6012eef8b542fa883ba115db731` | Merged with exact tested executable inputs preserved |

Older audit/recovery/qualification branch tips are already ancestors of the
combined source. The original branch's earlier managed-restore report is also
preserved. No completed work is left available only on an alternate Plan 5
branch. Historical reports keep their original branch and source facts; this
report supplies the current delivery location rather than relabeling old proof.

## Established integration defects and owned fixes

The preserving merge inserted two identical `audit_child_identities` methods
from the original and primary-integrated copies of the same fix. Removing the
extra method restores the reconciler byte-for-byte to its qualified saved-plan
source; it discards no original behavior.

The canonical test from the older branch injected item custody into a native
probe that now already contains that transition. Its fresh SQL probe failed
compilation with duplicate `old`/`next` declarations. The failed command,
original test and generated source are retained in the new evidence namespace.
The test now consumes the current probe directly and expects the saved reader's
three precise original-plan mismatch findings. Its previous false-clean
expectation conflicts with the completed saved-reader repair. All canonical SQL
refusals, native cases, schema checks, read-only checks and original counts remain.

Owned follow-up files are `scripts/reconcile_economy_accounting.py`,
`tests/async/test_economic_sql_canonical_audit.py`, and this report. No shared
native, coordinator, contract, migration, producer, registry/matrix or activation
file is independently changed. `AUDIT_OPERATIONS.md` preserves both completed
command descriptions. No interface/schema change is requested.

## Exact combined verification

The saved reader/exporter/fixture source hashes match the 159 passing Plan 5
methods reported in `PLAN5_SAVED_ORIGINAL_PLAN_QUALIFICATION_2026-10-05.md`.
The added canonical command's unchanged SHA-256 is
`765b0d81cfcfba5ae0e5a41e88a672625f178bfeeb1749396e41946ec29dfd2a`;
the repaired canonical test's SHA-256 is
`af75777b4fcdd0eee5b5a45e05e483c40d9a5b62428f666326fd66acfac4715a`.

With `PYTHONPATH=/workspace/tests/async` and `PYTHONDONTWRITEBYTECODE=1`:

- `python3 -u -B -m unittest -v test_economic_sql_canonical_audit.CanonicalAuditTests`:
  10 methods pass, zero skips, 0.308 seconds including subprocess overhead.
- `python3 -u -B -m unittest -v test_economic_sql_canonical_audit.NativeCanonicalAuditTests`:
  one method passes, zero skips, 184.571 seconds including subprocess overhead.
  Set `DURIS_PLAN5_CANONICAL_NATIVE=1` and
  `DURIS_PLAN5_CANONICAL_ARTIFACTS=/workspace/bin/tests/plan5-saved-plans-2026-10-05/native-consolidated-final`.

Both use immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Fresh SQL and flatfile native probe builds use the original strict C++20
ASan/UBSan recipe and no binary reuse. Both final probe hashes are
`96b48aab95d7b54baacd9e51f7326f0eaaa21512cc4fa37b836aeb6cf5915b15`.
Both private engines, MariaDB10.11.14 and MySQL8.0.46, apply canonical0056.
There are38 read-only API captures and38 CLI checks:20 accepted intact/repaired
cuts and18 refused cuts. All inventories remain unchanged, each read rolls back,
and both SELECT-only roles reject UPDATE. Six native-backed damaged projections
also yield the precise saved-reader child/posting/item findings. Complete command
receipt authentication and release qualification remain false.

The combined selected Plan 5 scope is170 distinct methods with zero skips.
The two shared source-contract failures remain: SHOP placement wrapper selection
and stale SQL route line887. Normal validator/matrix/inventory success and the
release validator's exit1 remain as reported. Their shared source inputs are
unchanged; this consolidation does not waive or independently repair them.

## Evidence, curator handoff and remaining gates

`bin/tests/plan5-saved-plans-2026-10-05` retains every attempt, source preimage,
command/source receipt, native build and SQL result. The fresh collector paths
remain `bin/tests/plan5-price-view-sql/saved-plans-collector-1` and `-2`.
`tmp/plan5/saved-plans-evidence.json` seals new artifacts plus all68,183 prior
artifacts, verified intact. `saved-plans-delivery.json` verifies final committed
owned bytes, source trees, branch ancestry and remote tip.

This is the curator handoff for the primary's locally maintained notebook;
its locality is not a blocker. The primary owns shared registration updates,
remaining source repairs, complete R1–R8 producer/authority/receipt and gameplay
journeys, both-backend retained restore/retention/service qualification, and
release-host budgets. The saved-reader report gives the narrow field/invariant/
consumer handoff and child pure count8-to15 registration request.

Accounting remains inactive. Wallet-root exclusions and the declined inactive
spell change remain. No production data, activation, deployment, PR merge or
independent push to `experimental-accounting` is performed. Future Plan 5 work
continues on `codex/accounting-plan5`; the primary integrates and publishes the
tested combined candidate.

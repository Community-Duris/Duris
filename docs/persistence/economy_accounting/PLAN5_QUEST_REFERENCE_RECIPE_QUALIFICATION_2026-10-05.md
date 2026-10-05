# Plan 5 current quest-reference recipe qualification

The current primary item command needs the native quest-reference codec. The
standalone canonical SQL audit fixture still used its older link recipe and
failed with unresolved native reference encode/decode functions. This slice
adds the existing codec to that owned fixture and qualifies its current source
on both native configurations and both private SQL engines.

## Source and ownership

- Delivery branch: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Refreshed primary: `1e36e8b06b51e30dc72e1d99824c1c4e425b2d0f`.
- Preserving merge/base: `fde86f849a323756e47a4a2980578e45c666040d`.
- Native tree: `d9fc5c96696a71904d2e18c5edf15cd48f0d9423`.
- Migrations tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, canonical0056.
- Result and verified remote SHA: `tmp/plan5/quest-reference-delivery.json`.

Owned changes are `tests/async/test_economic_sql_canonical_audit.py` and this
report. The recipe now includes `src/world/quest_mobile_native_reference.c`,
matching the primary's current child/collector fixture dependencies. No native
source, coordinator, accounting contract, migration, producer, registry/matrix
or activation file is independently changed. No shared interface change is
requested. Earlier work remains preserved on the same delivery branch.

## Exact evidence

The original source and failed command are retained in
`bin/tests/plan5-quest-reference-2026-10-05/canonical-test-preimage.py` and
`native-red.log`. Both SQL and flatfile final probe builds use C++20, strict
warnings, ASan/UBSan and the original flags, with no binary reuse. Their
SHA-256 is `0756e24bb418d1a6a88977488fc29ebe1a5897fb92ad39e34694b565d33d246a`.
The resulting owned test SHA-256 is
`de9f54e0839df005e78809cb96a5ce7f36637c6de1c16643336b0f7171f44bdc`.
The independent production reader/exporter/decoder retain their earlier hashes;
each command receipt records the exact current consumed source.

All checks use immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with a read-only repository, writable private `bin/` artifacts and no network.
Native database checks use the existing private namespaces. Set
`PYTHONPATH=/workspace/tests/async` and `PYTHONDONTWRITEBYTECODE=1`.

| Command | Result |
| --- | --- |
| `python3 -u -B -m unittest -v test_economic_sql_canonical_audit.CanonicalAuditTests` | 10 pass, zero skips, 0.373 seconds including subprocess overhead |
| `python3 -u -B -m unittest -v test_reconcile_economy_accounting.ReconciliationTests test_plan5_child_identity.ChildIdentityTests` | 129 pass, zero skips, 69.509 seconds including subprocess overhead |
| `python3 -u -B -m unittest -v test_economic_sql_canonical_audit.NativeCanonicalAuditTests` | One pass, zero skips, 196.483 seconds including subprocess overhead |

The native selection sets `DURIS_PLAN5_CANONICAL_NATIVE=1` and
`DURIS_PLAN5_CANONICAL_ARTIFACTS=/workspace/bin/tests/plan5-quest-reference-2026-10-05/native-final`.
Fresh MariaDB10.11.14 and MySQL8.0.46 both apply canonical0056. There are38
read-only API captures and38 CLI checks:20 accepted intact/repaired cases and18
refused damaged cases. Native child cases, SQL constraints, SELECT-only roles,
rollback and unchanged inventories remain checked. Six damaged projections also
retain their exact saved-reader original-plan mismatch findings. This slice's
current selected scope is140 methods; older component/build results remain
qualified only for their recorded source, rather than being counted here.

`tmp/plan5/quest-reference-evidence.json` seals new artifacts and records the
inherited prior manifest without claiming a new full rehash of old archives.
`quest-reference-delivery.json` verifies the owned committed bytes and remote tip.

## Remaining gates and curator handoff

Source review separately found that the independent EAP1/EAB1 readers still
limit custody owners to11, while the primary now supports native-mobile owner12
with its original identity/context and bounded equipment rules. Native-backed
reader parity for that grammar requires a separate established defect/fix slice.
This fixture's player-to-player plans do not establish native-mobile audit
coverage or quest gameplay completion.

The previously reported shared SHOP wrapper/SQL route contracts, maintained SQL
build expression, publication and managed recovery gates remain open; they are
not newly executed or waived here. Complete native capture, receipt/admission
binding, all writer/producer journeys, restore/retention/service and release-host
budgets remain required for R1–R8. Accounting remains inactive, wallet-root
exclusions and the declined inactive spell change remain, and no production
data, activation, PR merge, deployment or independent experimental-accounting
push occurs. This report is the curator handoff for the primary's locally
maintained notebook; notebook locality is not a blocker.

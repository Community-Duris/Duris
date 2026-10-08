# Plan 5: zero-net ordinary account history - 2026-10-08

The independent reconciler now accepts the existing native contract for an
ordinary account effect whose nonzero postings offset to zero, retaining both
balance and revision. It also orders that effect before an advancing effect at
the same starting revision, regardless of operation ID. Invalid original plans,
backward clocks, changed balances without an advancing clock, posting mismatches,
opening discontinuities and stale authority still refuse. This is a reader
correctness slice. Genuine producer reachability and full release remain open.

## Delivery and ownership

- Local/remote branch: `codex/accounting-plan5`; no branch switch in this slice.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base: `ce23baf26acbeee486b020f118cd03d0cb9ee9d4`.
- Result: this containing commit, recorded exactly with remote equality in
  `D:/Dev/Tests/Duris/accounting-plan5/zero-net-20261008/delivery/result.json`.
- Code: `scripts/reconcile_economy_accounting.py` (`audit_accounts` only),
  `tests/async/test_reconcile_economy_accounting.py` (one reused model helper and
  four regression methods), and the existing
  `tests/async/run_economic_sql_audit_snapshot_mysql.py` (one focused helper/call).
- Documentation: `AUDIT_OPERATIONS.md`, this report and the additive remote follow-up.
  `scope-binding.json` authenticates every untouched AST and all 27 overlay blobs.
- All seven prior branch tips and the prior tombstone fix remain ancestors; the
  post-push receipt verifies this. No work is discarded or force-pushed.

No shared interface/schema change is requested. Existing `before`, `after`,
`before_revision` and `after_revision` fields retain their native meaning.
`economic_coin_effects_validate` permits a nondecreasing clock for equal balances,
and requires an increasing clock when balance changes. An effect not referenced
by any posting must have equal balances and an increasing clock. The independent
plan decoder already enforced those rules; the account-history reader now agrees.
Monotonic revision jumps remain valid. Item revision rules are unchanged.

The integration owner should preserve these four existing-class methods when
updating its maintained test registration:

- `ReconciliationTests.test_zero_net_money_effects_preserve_revision`
- `ReconciliationTests.test_zero_net_money_effect_precedes_revision_advance`
- `ReconciliationTests.test_zero_net_money_invalid_history_still_refuses`
- `ReconciliationTests.test_unreferenced_money_effect_requires_revision_advance`

## Exact source and defect proof

Published primary at initial reproduction:
`fc8a8961b281c4b53bc0f7af1459dbb131781676`; production/test inputs did not change
when primary advanced to `5f5a8bdfd0306a6c65857cf936fd6b332832c90b` (three documentation
files only). Red and fixed candidates pin the latter primary plus the same 27
owned overlays, including the unchanged earlier backup and tombstone fixes.
The result commit binds the tested owned blobs. The historical native tree on
this branch is not substituted for the published native tree in these tests.

| Source | Composed tree | Archive SHA256 |
| --- | --- | --- |
| Unchanged reader, 00 | `0991fdbbb06e6bef9bebf03d15677d582201e1fe` | `fa64cdc9a80b26a70be97b009245bf939c71f24931f2540f878468b89e1631d1` |
| New regression methods, old reader, 01 | `9cf3a4563eb5071627c86db894cd78b466cf8e1e` | `9c70f3bd31314cb5ff8553c23bf7d0b5351501e6c2f64ca0b880a2bb6fd8eb2b` |
| Complete fixed slice, 02 | `b34fbb29dfc91c9e8a241044ed4649b634ff5aa7` | `5e5ba88aac6abaca25bc48b493bb33a9159327a9fc57d999c5dc376f8c499c83` |

All three authenticate 6,512 Git blobs: 6,508 regular files and four exact link
targets. Native tree: `833d3085815b396861ad18a77635412212381e4b`.
Migration tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.
Canonical head: `0064_auction_custody_history`. Source body/mode/link guards
passed after every stage. Fixed reader blob:
`def95a4221a4216d101420e587597974e0d9eeb5`, SHA256
`975558ab7e9edb4e661b6409982352b8e71d311d0502516614a1162b1151bb20`.

The unchanged reader raises one false `broken_account_history` for each of four
valid equal-clock cases (0, 1, 2^63 and UINT64_MAX). A valid mixed chain whose
advancing operation sorts first by ID raises two `broken_account_history` and
one `stale_native_balance`. The independent decoder accepts all these plans.
Task-only probes linked to 14 genuine published native providers accept and
exactly re-encode their six valid plan inputs in both SQL and flatfile builds;
both reject backward, changed-same-clock and unreferenced-same-clock controls.
The 18 original probe executions comprise 12 acceptances and six refusals.
These are explicitly model-authored plans, not admitted producer journeys.

The four new methods on the old reader give eight failed subtests, zero errors
and zero skips; terminal exit1 and raw failure output are preserved. The fix
changes only the native clock predicate and the account-history tie order.

## Executed checks and backends

Tools image: `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
GCC13.3, Python3.12.3, nm2.42 and mysql_config10.11.14 are version/body-bound in
`seal/native-build-inventory.json`. All builds/evidence are on D:, mounted
directly. The checks stage uses a dedicated RAM filesystem for private databases;
the original canonical method uses its required private root path in the fresh
container layer on the existing D: Docker disk. There are no network/host ports
or existing services/volumes. Sanitizers, original strict flags and deadlines
remain in force. No repository C/C++ or native recipe is changed.

| Stage | Exact invocation within tools image | Result |
| --- | --- | --- |
| `reproduce00` | `python3 -u -B /evidence/reproduce00/observer.py reproduce00 00` | Old reader false findings, two fresh native probe builds, 18 codec checks; 76.389871s |
| `red01` | `python3 -u -B /evidence/red01/observer.py red01 01` | Four methods, eight failures, zero errors/skips; expected exit1; 0.398680s |
| `checks02` | `python3 -u -B /evidence/checks02/observer.py checks02 02` | 16 modules: 396 loaded, 378 PASS, 18 existing opt-in skips; full snapshot runner PASS on both engines; 190.246936s |
| `canonical02` | `python3 -u -B /evidence/canonical02/observer.py canonical02 02` | Existing complete native restore method PASS, zero skips, both fresh canonical0064 engines; 417.735174s |
| `codec02` | `python3 -u -B /evidence/codec02/codec-observer.py codec02 02` | 50 original native codec checks of nine model/16 captured SQL plan inputs across both modes; 20 accepted/30 refused; 3.074691s |

Each stage's `docker-command.json` has the complete actual host command,
resource limits, mounts and image. `commands.json`/`launches.json` retain exact
argument vectors, deadlines, exit status and timings, including expected
nonzero refusal commands. The checks observer lists all 16 actual modules and
uses their existing unittest loaders; audit budget opt-in is enabled. Its SQL
subprocess is `python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`
with generated private socket/disposable-schema settings. The original native
method is `RestoreCoinEffectsTests.test_native_coin_effects_both_modes_and_canonical_engines`,
with `DURIS_RUN_RESTORE_COIN_INTEGRATION=1` and `DURIS_PLAN5_CANONICAL_EVIDENCE=1`.
The 18 skips belong to the pure suite's pre-existing separate opt-in methods,
not to the separately enabled complete canonical method.

MariaDB: `10.11.14-MariaDB-0ubuntu0.24.04.1`; MySQL:
`8.0.46-0ubuntu0.22.04.4`. For each engine the existing complete partial-snapshot
runner adds eight modeled SQL cuts (two operation-ID orders x valid/backward/
changed-same/unreferenced-same) and 24 actual CLI checks at limits0/1/100.
Valid additions preserve the original exception counts of the existing partial
fixture; they do not turn that fixture into a clean or complete economy.
All 47 application tables remain byte/value-inventory equivalent across capture,
audit and CLI, with SELECT-only reader credentials, one rollback/closed cursor
per new capture, and exact fixture restoration. All 16 new captured plans are
subsequently checked by both original native codec builds. The authenticated
probe binaries are reused against the identical native tree and preserved bodies.

The existing canonical native corpus agrees with the independent decoders on
3,026 cases, 1,054 accepted, identically in SQL and flatfile modes. Each fresh
canonical engine completes 109 cuts/90 expected refusals, including 58 full-entry
restore cuts, the original pending-claim/intents/history cases and original
bounds. Authority remains unchanged. Parent observation records 18 successful
g++ driver calls: two fixture compilation/link commands and 16 toolchain queries;
this count is not 18 compilation jobs. Nested runner compilation totals are not
inferred from the parent observer. The two task-only probe builds are separate.

## Evidence, integration and remaining gates

Evidence root: `D:/Dev/Tests/Duris/accounting-plan5/zero-net-20261008`.
Helpers: `D:/Dev/Temp/accounting-plan5-zero-net`.
Builds: `D:/Dev/Builds/Duris/accounting-plan5-zero-net-20261008`.
`qualification.json`, source00/01/02 manifests, transport authentication,
`scope-binding.json`, raw reproduction/red output, complete unit/engine logs,
model/SQL plans, native inventories and command records are retained.

Raw seal SHA256: `fd5aeb200401d8ef5bc0d90883257606e89bdd80b1dae22539974f5c20232dcf`. It covers 1178 files/996628362 bytes,
including 905 regular files under the build roots. Six containers are
stopped with no OOM or live recorded children; red01 retains its expected exit1.
There are no copied links or reparse points. Post-seal report/command-count
clarification/delivery artifacts are additional inputs, separately bound by the
post-push receipt; they are not retroactively counted in that raw seal.

Earlier tombstone fix `c373f04fac0ef14f734bc086c1cef01246dc70fe`, restore blob
`23b17a3d158ac455105de1cd1303561dcc028ad8`, is included in these 27 overlays and
preserved on the remote branch; primary still needs to preserve/import it with
its backup integration. Earlier 24-overlay evidence keeps its original scope.
No new schema, native API or producer change is requested.

The unchanged shared flatfile-authority fixture still has seven unresolved
genuine-provider symbols; its nine missing-record controls and full 12-method
backup qualification remain open as documented by the previous slice. The
room-seed genuine UID-owner link and original native SQL baseline recipe/head
closure also remain primary-owned. They are not re-run or relabeled here.
Complete genuine producers, admission/recovery/ACK, cold native world and wallet
lifetimes, complete openings, registry/matrix, player/route/fault/load journeys,
the unpublished combined candidate, activation-owner qualification, Plan5 and
R1-R8 release remain open. No maintained whole-server rebuild or gameplay boot
is claimed for this Python-only reader change.

This report and remote follow-up are additive curator-ready notebook inputs.
The primary-local notebook remains nonblocking; curator application/acknowledgment
is not claimed. Inactive behavior, wallet-root ITEM_MONEY exclusions and the
declined inactive spell-path change are preserved. No activation, autocorrection,
production access/write, experimental-accounting push, deployment or merge.

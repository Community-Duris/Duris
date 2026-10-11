# Plan 5 independent held-policy acceptance review — 2026-10-11

The actual held-retirement policy entry passed in SQL-header and flat modes on
published primary97. Its registered acceptance contract still rejects the
original receipt: **`missing required cases: main`**. The receipt observes
`main` as called, but the original entry adapter does not emit it as a case.
The separate native collector exception is retained; no native rerun, invented
case, shared edit or full durability/release claim is made by this review.

## Exact source, owner and results

- Plan5 branch/remote: `codex/accounting-plan5`; clean owned worktree
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Initial review publication: base `e9225b07ba745cc7d73961b9d018f39ad7e723c7`
  to result `77dec0864f6b16e2178123c61bec06650e1852a4`.
- Corrected handoff base: `77dec0864f6b16e2178123c61bec06650e1852a4`;
  result/remote identities are bound afterward in the separate
  `delivery/handoff-correction-publication.json`. The original review,
  publication receipt and native evidence remain immutable.
- Native source: `97ded8571375f3fdc6168487a13120f65caedbec`, tree
  `0ae109dc0c8daa3f35ebd988f0d35995e089278e`.
- Original native owner: coordinator chat
  `01a129b3-9c1a-77b3-a10a-3ddafdb5c0a0`, job `ACC-07-HELD-POLICY-01`.
- Pinned QA image:
  `sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`.
- Actual original entry: exit0, 15.478887962992303 seconds, empty stderr,
  `held-retirement publication policy sql: PASS` and
  `held-retirement publication policy flat: PASS`.
- Original receipt: `complete=true`, `cases=[script passed]`, `called=[main]`,
  `collected=[]`. Both required mode markers are present; skipped checks0.
- Independent unchanged acceptance checker: `missing required cases: main`.
- Original outer Docker exit1: `SimpleNamespace` has no `skipped_checks`.
  That collector exception occurred after the native entry and before the
  original cohort's source-contract command. The latter is **NOT_RUN** in that
  sealed packet; subsequent coordinator work has its own evidence and scope.
- Maintained owned files: this standalone review and additive Plan5 follow-up.
  No runtime, schema, producer, registry/matrix, activation or shared runner edit.

The coordinator executed the unchanged registered entry:

```text
python3 -B /opt/source/tests/run_test_entry.py /opt/source/tests/async/test_held_retirement_publication_policy.py script /evidence/entry-cases.json 1
```

That original script compiles and runs the real extracted policy with its typed
boundary harness in both modes. Its ACK/physical bindings are simulated; the
SQL mode is a header/build selection, not a database-service journey. It does
not establish real journal acknowledgement, locking, persistence, whole host,
cold restoration or release. No retained executable claim is made: the original
script's temporary binaries were removed by its own context manager.

## Independent proof and preserved failures

Plan5 verified all3,718 frozen files against primary97 Git blobs and both exact
source archives, all3,748 sealed files/213,498,189 bytes, and the complete20-file
freeze. The sealed producer packet was read without mutation.

The review imported the original complete `run_integration_matrix` and
`run_regression_tests` modules from that frozen source, used their actual
`TestResult` type, computed skipped checks with original `skip_count`, and called
unchanged `outcome_contract` with the actual original output and case receipt.
No test entry, native compile, validator, integration matrix or database was
rerun. The checker rejection is an executed result, not an inference from
manifest text.

| Original checker input | Git blob | SHA256 |
| --- | --- | --- |
| `tests/run_integration_matrix.py` | `6435fb4b2e989d42682a28f9b6b39c1a2ac98b9d` | `dff0c6962c75b5f6f1c64d301be9cc4d5938ce86b2fa814cc6bba652a7502f4d` |
| `tests/run_regression_tests.py` | `4895e9801125aeaa1ca04f825005393a708915e7` | `8423122380f527dcba31f473bb4dc0a28e2c72172a3f025349a2660fddba7a10` |
| `tests/run_test_entry.py` | `4429201755e89787fd7231b61fa0232e1058bafe` | `5f00f0770a82948e6e031cb6a4d5478a55e5e97de0a2779a98359581cb929633` |

Independent Windows Python3.12.10 command:

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B D:/Dev/Temp/accounting-plan5-ACC07-peer-review-20261011/review_terminal.py
```

First review: exit1 after the original checker ran, because the review indexed
the saved inspection JSON object as an array. The original helper, actual
traceback and failure receipt remain in the separate
`D:/Dev/Tests/Duris/accounting-plan5/ACC-07-HELD-POLICY-01-review-failure-01`.
Original failed helper SHA
`d84ce85fd62b59c6cdeb4f9ee64ff5f8061fd53f2f38b7ebb66991ed30dc70aa`.
Only that review parser was corrected; source, assertions and producer evidence
were not edited. Corrected review exit0, 4.7794073 seconds; helper SHA
`a726b7e2f24cd2eeaa3aa6eb6e56a773ebc587cfade5f549621694b733b6e056`.

Review receipt:
`D:/Dev/Tests/Duris/accounting-plan5/ACC-07-HELD-POLICY-01-review/terminal-packet-review.json`,
SHA `6f0dee3f9af32b32bb010fdd505017aeec452c9890f2993d82876c96610befb3`.
Producer evidence:
`D:/Dev/Tests/Duris/accounting-accel-integration/ACC-07-HELD-POLICY-01`;
original seal SHA
`bae974ab1b2138757c41bda61bb049af72e94dfce0cb0dbf725750f3f609e20e`.

## Narrow primary-owned acceptance handoff

Consumer: `tests/run_integration_matrix.py::outcome_contract`.
Existing row: `tests/integration_manifest.json`,
`id=held_retirement_publication_policy`, `mode=script`,
`required_cases=[script,main]`, both existing `required_markers` shown above.
Invariant: every required case must be in the original `cases` receipt with
status passed. `called=[main]` alone must not be promoted to a passed case.

The original adapter at lines157–168 rejects an uncalled declared `main`, then
emits `script` and only invoked `test_*` function outcomes. The coordinator's
published `f280cebb99515cea2bc53ccc9a1a62d349905a93` handoff explicitly retains
both named cases. This corrected request therefore preserves
`required_cases=[script,main]`, original mode, both mode assertions and markers.
The initial report's optional `[script]` registration suggestion is withdrawn;
it is not the source adoption request and must not bypass the named contract.

Primary owns the shared source decision in `tests/run_test_entry.py` and any
associated runner tests. Existing receipt fields `cases[].id`, `status`,
`elapsed` and `evidence` must describe genuinely observed `main` invocation and
outcome under the original registered entry. Retain actual `called`, `complete`,
the `script` case, native assertions and existing rejection behavior. A raw
`called=[main]` entry must not be copied into a fabricated passed case. No new
receipt schema, runtime interface or migration field is requested.

Validation must bind the adopted exact source, unchanged original registered
entry, actual complete/called receipt, both mode markers and unchanged
acceptance checker. Check that missing invocation, failing native execution,
missing mode evidence or skipped required work still rejects acceptance, and
that both named cases have actual successful outcome evidence. A collector
adapter must use the real original `TestResult`, including `skipped_checks`;
fixing the result shape alone does not solve missing `main` evidence.
The old native pass, collector exception and registration rejection remain
immutable; corrected source is not retroactive acceptance of the old receipt.
No new schema or runtime interface fields are requested.

## Resource closure and remaining gates

Independent review verified exact image/grant/CID/direct-D mounts, original
read-only source/helpers, foreground `--rm`, restartno, sealed evidence and
guarded release. Original CID
`70283568c6b20f82342ae3802c8c12fba9a67c970b8e746b5757db02986b8b36`
is now absent. Seven unrelated running service IDs are unchanged. Coordinator
cleanup retains all four passive volumes without consumers. Plan5 started0
containers/jobs/databases and acquired no resource lease for this review.

The independent report is additive curator input; primary/notebook adoption is
unconfirmed. The coordinator has published the source-only restore-coin report
and patch in `f280cebb99515cea2bc53ccc9a1a62d349905a93`; that does not adopt the
candidate test module or establish native/DB qualification. That packet remains separately pinned;
current full native/SQL restore and coherent0062/default65 route remain open.
Real native holdings/UID authority, all-writer qualification, both-engine
durability, backup/restore, retention, erasure, gameplay, faults, recovery,
measured workloads and full Plan5/R6–R8/release acceptance remain OPEN.
Accounting inactive/CLOSED, release BLOCKED, full goal ACTIVE. Preserve wallet
root exclusions and the declined inactive spell path; no activation, deployment,
merge, production operation, audit auto-correction or experimental-accounting push.

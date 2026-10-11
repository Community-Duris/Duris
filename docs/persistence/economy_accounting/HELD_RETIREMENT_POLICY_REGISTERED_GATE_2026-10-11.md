# Held-retirement policy entry passes; registered case contract rejects its receipt

On published primary `97ded8571375f3fdc6168487a13120f65caedbec`, the original
held-retirement policy entry compiles and executes its SQL-header and flat
fixtures successfully. The original integration acceptance checker rejects the
result because the registration requires a `main` case, while the original entry
adapter records `main` only in `called`. This is a reproducible case-observation
contract failure, not a demonstrated policy runtime failure or SQL durability
result. No acceptance requirement was removed and no runtime source was edited.

## Exact source and execution

Coordinator job: `ACC-07-HELD-POLICY-01`; owner chat
`01a129b3-9c1a-77b3-a10a-3ddafdb5c0a0`.

- Frozen primary commit: `97ded8571375f3fdc6168487a13120f65caedbec`.
- Frozen tree: `0ae109dc0c8daa3f35ebd988f0d35995e089278e`.
- Previous primary: `9c871c875b7d794434e0f636515e3fb7caf44f78`.
- Exact Git inputs: 3,718 regular files; the complete changed 20-file batch is
  pinned separately in `twenty-file-freeze.json`, SHA256
  `9c14a0294c642254a4c54ca72d7acc377c60583ea152c0256804d53de9d21a52`.
- Tool image:
  `sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`.
- Original script and original C++ harness were read-only frozen inputs. No
  fixture, compiler flag, native assertion, required marker or mode was changed.

The actual unmodified entry command was:

```text
python3 -B /opt/source/tests/run_test_entry.py /opt/source/tests/async/test_held_retirement_publication_policy.py script /evidence/entry-cases.json 1
```

It started at `2026-10-11T12:48:59.956629+00:00`, exited **0** after
**15.478887963 seconds**, and produced both original markers with empty stderr:

```text
held-retirement publication policy sql: PASS
held-retirement publication policy flat: PASS
```

The receipt has `complete=true`, one passed `script` case, no collected unittest
cases, and `called=["main"]`. Its native elapsed value inside that case is
15.352103236 seconds. These are one script's two native modes; no invented count
of separate native assertion cases is reported. The original temporary-directory
cleanup removed the executables; no retained native-binary hash is claimed.

## Actual original acceptance rejection

The published `held_retirement_publication_policy` registration in
`tests/integration_manifest.json` requires both `script` and `main` cases and the
two markers above. `tests/run_test_entry.py` observes the `main` invocation, but
appends outcomes only for `script` and called top-level `test_*` functions.
`tests/run_integration_matrix.py::outcome_contract` checks case records, not the
separate `called` list.

The unchanged checker was executed on the retained real receipt and output using
the unchanged `tests/run_regression_tests.py::TestResult`, with its original
`skip_count` returning zero. The exact rejection is:

```text
missing required cases: main
```

Entry exit, passed `script`, completeness and both markers are satisfied. No
native rerun, full integration-matrix invocation or production build was needed
to reproduce this rejection. The checker receipt is
`completion-review/original-outcome-contract.json`, SHA256
`bac4d59bb8c7ff09200e41e9617a120f6c5254831bcf333a5f7cb773ad98809b`.

Primary owns the compatible source/registration decision. Preserve actual
`main` invocation, both required-case obligations, original mode assertions and
markers. Do not manufacture a passed case from a call alone or delete an
acceptance requirement. A repaired exact source must satisfy the original entry
and unchanged acceptance checker before this registered gate can close.

## Source review and focused contract check

Independent source comparison finds whitespace-insensitive equality for the six
extracted method bodies, the pending-movement representation and publication
state. All 931 writer policies are unchanged apart from lexical site coordinates;
census remains 2,911. The 13 changed source pins match the frozen published bytes,
and the other 481 pins remain unchanged. This is bounded source preservation,
not whole-host semantic, ABI, layout or architecture qualification. An independent
changed-source formatting gate was not executed in this cohort.

The original default `scripts/validate_economy_accounting.py --root <freeze>`
was subsequently executed once under Windows Python 3.12, exit0 in 6.5 seconds:
14 fixtures, 931 writer routes, 2,911 candidate sites, `release_ready=False`.
Its exact receipt is `completion-review/source-contracts.result.json`, SHA256
`804ab3025e270e3b1ccba5662c2c58e53d414f0d3187ab97de1108ec33efc5e9`.
The release switch was not rerun. The earlier
[9c release refusal](PLAN5_CURRENT_RELEASE_GATE_AUDIT_2026-10-11.md) keeps its
original source pin and proof limits.

## Preserved failures, independent review and resources

The original outer Docker/host job exited1 because the coordinator collector
passed an incomplete `SimpleNamespace` to the checker and raised
`AttributeError` for absent `skipped_checks`. The native entry's original exit0
and raw receipt are retained separately. Source contracts were NOT_RUN before
that exception. Completion used the original result type and retained receipts;
it did not repeat the native compile/runtime. Preparation `KeyError: sources`,
supplementary pin-review `FileNotFoundError`, and a CRLF comparison assertion are
also preserved with their correction scopes. They do not relabel the native
result or erase the registration rejection.

Plan5 independently verified all 3,718 inputs against exact Git blobs, both
archives, the 20-file freeze, all 3,748 initial seal members/213,498,189 bytes,
original exit receipts, and the same rejection using the real original result
type. Its receipt is
`D:/Dev/Tests/Duris/accounting-plan5/ACC-07-HELD-POLICY-01-review/terminal-packet-review.json`,
SHA256 `6f0dee3f9af32b32bb010fdd505017aeec452c9890f2993d82876c96610befb3`.
Its initial JSON inspection-shape review failure is preserved separately; no
native/database job was repeated or started by the reviewer.

Evidence root:
`D:/Dev/Tests/Duris/accounting-accel-integration/ACC-07-HELD-POLICY-01`.

- Original resource/evidence seal:
  `bae974ab1b2138757c41bda61bb049af72e94dfce0cb0dbf725750f3f609e20e`.
- Completed result:
  `a26db35f638d77da987394837e41a1de345cbbb93808b4ca34f9ee533a86a2da`.
- Completed seal: 3,761 members/214,182,814 bytes, SHA256
  `2bb64f16adf7711d70678022559bf39dfd8c66df75cd1b3178162584d9fcfca2`.
- Exact resource release:
  `9e53b48193254839a59de47dd2d1003dbee9c6dbacfdd6da0f359cd882a21af3`.

The sole disposable QA used 1GiB/CPU1, restart=no, no network, read-only source
and helper mounts, and five direct D: mounts. Generated temporary binaries were
under the task's D: build directory. Exact CID
`70283568c6b20f82342ae3802c8c12fba9a67c970b8e746b5757db02986b8b36`
was removed; its exclusive lease released. All seven unrelated service IDs and
four passive diagnostic volumes without consumers were preserved. No database
service, game, production, deployment or activation operation occurred.

This qualifies isolated policy assertions with simulated repository, ACK and
physical bindings in SQL-header mode and the flat refusal. It does not qualify
real SQL, journal durability, ACK locks, cold restoration, shutdown, gameplay,
full runtime/ABI, 32MiB storage, whole architecture or release. Accounting remains
inactive/CLOSED, the registered gate remains rejected, and full acceptance OPEN.

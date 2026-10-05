# Plan 5 independent child identity reconciliation qualification

The read-only reader could report a clean reconciliation for zero, nonhex,
incorrectly derived or reused child operation IDs. Its SQL exporter also omitted
the existing facts needed to independently derive those IDs. This slice repairs
that defect without changing a native mutation codec, persisted schema, producer,
writer registry, coordinator or activation policy. Full release remains BLOCKED.

## Candidate and ownership

- Branch: `codex/accounting-plan5`, published separately from
  `experimental-accounting`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `d90381b549960dce652a588d14422eeac0e90dca`.
- Primary publication consumed by that base:
  `b67c1fb0defb07cc0a088a47723ec647d85c6db0`.
- Exact native tree: `ab5e68c90f00268b62c4b811c36b46fcfc4e4db7`.
- Exact migration tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`,
  through canonical `0056_spell_ward_durability`.
- The result SHA and verified remote tip are recorded after commit in
  `tmp/plan5/child-identity-delivery.json` and the slice delivery.

Owned changes are `scripts/reconcile_economy_accounting.py`,
`scripts/economic_sql_audit_snapshot.py`,
`tests/async/test_reconcile_economy_accounting.py`, new
`tests/async/test_plan5_child_identity.py`, the child snapshot contract in
`AUDIT_OPERATIONS.md`, and this report. All pre-existing unowned tracked inputs
remain byte-identical to the base. No shared interface or schema change is
requested for child identity. Central test registration remains a narrow
primary-owned coordinator integration request below.

The final refresh observed primary
`6a78031fbd521d350bef29bd4d9044d6b457ba74`, native tree
`5ca91c7f3369df5ce655af15bf3ffb6afba7ef20`, with the same migration tree.
Its newer shop recovery module, Makefile change and central test inventory are
outside this frozen candidate. The primary imported the preceding census and
pin-repair slices with their original qualification limits. Neither those
imports nor this report qualify the newer combined native source.

## Established defect and complete scoped repair

`red-results.json` retains five preimage snapshots for zero, nonhex,
wrong-derived, duplicate and absent metadata cases. The original reader returned
zero findings for every one. The eight new pure regression methods produced
47 expected assertion failures against that preimage, with zero test errors;
the complete red transcript is retained.

The independent production reader now checks every raw child row before slot
indexing. A duplicate slot cannot hide malformed derivation evidence. It checks:

- Canonical lowercase 16-byte nonzero IDs, distinct from the root, and child ID
  uniqueness throughout the captured rows, including different roots.
- Original persisted `domain_id` in `1..UINT32_MAX`, `discriminator` in
  `0..UINT64_MAX`, integer `relationship == 1`, and all four newly projected
  fields present, including an explicit nullable `receipt_operation_id`.
- `parent_index == 0` for the root or an existing earlier child of the same root.
- Independent derivation: the first 16 bytes of SHA-256 over the original parent
  ID, little-endian uint32 domain, and little-endian uint64 discriminator.
- A nonnull receipt operation ID is canonical and equals the child ID.

Existing strict slot types/ranges, dense indexes, evidence counts, root linkage
and posting/item-reference child linkage remain in force. New findings are
`missing_child_identity_evidence`, `child_identity_mismatch` and
`duplicate_child_operation`; malformed facts use `invalid_child_link`.
Checks run independently of display limits and operation filters.

The SELECT-only exporter projects the original four fields from
`economic_accounting_child`; it neither derives nor repairs stored facts.
The existing operation view already accepts these numeric/ID fields. Older
exports with children and absent derivation metadata produce explicit findings.
The production implementation imports only the standard-library hash function,
not the mutation validator or codec. Native helpers are confined to the test
probe process.

This repairs the native child-link identity/derivation contract. It does not
authenticate a child receipt's command/status or compare every detail row with
the immutable parent EAP1 bytes. A coherent rewrite of derivation facts and ID
still requires that separate original-plan authentication. These limits remain
release gates; no clean component result is promoted to release completion.

## Exact source and execution environment

| Final owned input | Raw SHA-256 |
| --- | --- |
| `scripts/reconcile_economy_accounting.py` | `5bf3507a059b87132498cf7295fb7689de92a76b1f932e1dc95f4420dfe5fcce` |
| `scripts/economic_sql_audit_snapshot.py` | `a4e2cfa9bbcca46830517239c3cc6f086b87a4fd688d8414fc73d93fc093273b` |
| `tests/async/test_reconcile_economy_accounting.py` | `9ef8032f585e89c1e63b4865c19d29ac97f9cd461f723e51b53614cc3446e644` |
| `tests/async/test_plan5_child_identity.py` | `aea35bf71c455145943af0d1546b9b0e6894949994eeef0d4894a1a29f77840f` |
| `AUDIT_OPERATIONS.md` | `9f319e5a659799c8d258dd10b91e3fc59ed273f1ce167284ed2d19e197f14671` |

All runs use `duris-plan5-origin-sql-tools:local`, immutable image ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`:
Ubuntu 24.04.4, GCC 13.3, Python 3.12.3, OpenSSL 3.0.13, MariaDB
`10.11.14-MariaDB-0ubuntu0.24.04.1`, and MySQL
`8.0.46-0ubuntu0.22.04.4`. Containers use `--network none`. The repository is
mounted read-only at `/workspace`; only `bin` and, for the evidence recorder,
`tmp/plan5` are writable.

## Validation and retained commands

`bin/tests/plan5-child-identity-2026-10-05` is the fresh evidence namespace.
The copied `run-child-identity-checks.py` retains exact command arrays, opt-ins,
exit codes, durations and full transcripts in each `*-command.json` and `.log`.
The following are the successful selected commands, run from `/workspace` with
`PYTHONPATH=/workspace/tests/async` and `PYTHONDONTWRITEBYTECODE=1`:

```sh
python3 -u -m unittest -v \
  test_plan5_child_identity.ChildIdentityTests \
  test_reconcile_economy_accounting.ReconciliationTests \
  test_economic_sql_audit_origins.ItemRevisionTests \
  test_economic_sql_audit_origins.OriginTests
python3 -u -m unittest -v test_plan5_child_identity.NativeChildIdentityTests
python3 -u -m unittest -v test_plan5_child_identity.ChildIdentityBudgetTests
python3 -u -m unittest -v \
  test_economy_writer_coverage_contract test_audit_accounting_invariants
python3 scripts/validate_economy_accounting.py
python3 scripts/generate_economy_writer_coverage.py --check
python3 scripts/validate_economy_accounting.py --release
```

The native method requires `DURIS_PLAN5_CHILD_IDENTITY_NATIVE=1` and a fresh
`DURIS_PLAN5_CHILD_IDENTITY_ARTIFACTS` directory under `bin`. The budget method
requires `DURIS_PLAN5_CHILD_IDENTITY_BUDGET=1` and its separate fresh
`DURIS_PLAN5_CHILD_IDENTITY_BUDGET_ARTIFACTS`. Their exact values are retained.
The native SQL red/green comparison also requires the original base versions
of `reconcile_economy_accounting.py` and `economic_sql_audit_snapshot.py` in the
parent of its fresh artifact directory. The retained preimages come from the
base Git objects, not an installed package or a mutable upstream checkout.
For reproduction, materialize those two `scripts/` blobs from the exact base
before mounting the workspace. Probe reuse is optional; without its opt-in the
method compiles both configurations from scratch.

| Selection | Observed result |
| --- | --- |
| Pure reader, origins and item-revision regressions | 147 passing methods, zero failures/errors/skips; final run 72.022 seconds in unittest, 72.314675 seconds outer |
| Native/private SQL method | One passing method, zero skips; 130.327 seconds in unittest, 130.557656 seconds outer |
| Bounded workload method | One passing method, zero skips; 3.144822 seconds outer |
| Writer contracts and invariants | 71 passing methods, zero failures/errors/skips; 11.933 seconds in unittest, 12.333056 seconds outer |
| Ordinary contract validator | Exit 0; 14 fixtures, 887 routes, 2843 sites, `release_ready=False` |
| Matrix check | Exit 0; 887 rows, 879 function anchors, 2784 unique sites, zero unmapped; `coverage_complete=False`, release BLOCKED |
| Release validator | Expected exit 1: `writer has no executable evidence` |

These are 220 distinct selected passing unittest methods, not 220 observed native
writer journeys. A final rerun of the eight pure methods passed after the test
harness corrections. `py_compile` passed on all four owned Python inputs;
bytecode was confined to the ignored evidence directory. `git diff --check`
passed. No maintained C/C++ input changed, so no new maintained server rebuild is
claimed. The preceding SQL/flat builds retain their original native-tree scope.

### Native contract and real SQL cuts

The generated C++ probe creates and encodes a real native bank-transfer EAI1/EAP1
plan with two account effects, two postings and nested child links. It exercises
the uint32 domain and uint64 discriminator boundaries. Both SQL and `__NO_MYSQL__`
configurations pass seven native cases each: valid, zero, root-self, duplicate,
wrong discriminator, wrong domain and wrong parent.

The native commands use C++20, `-Wall -Wextra -Wpedantic -Werror`, ASan/UBSan,
`-fno-omit-frame-pointer -fno-pie -no-pie`, and the eight original translation
units listed in `native-builds.json`. Both binaries have SHA-256
`a13cb103a5f8b328f6d1816d002b8a4b6363ae330fe9f2fc20d898aca2cc106b`.
The successful final run reuses the previously compiled probes only after
verifying their generated source, exact flags/source list, every native input
hash and binary hash. It executes both again with empty sanitizer stderr. The
original successful compile logs and commands remain retained.

Each SQL engine uses a fresh private datadir and Unix socket with TCP disabled.
Bootstrap adoption and authoritative migrations end at exact sequence 56,
`0056_spell_ward_durability`. The native EAI1/EAP1 bytes and child rows are seeded
into the actual schema. The modeled holdings/opening origins are explicitly
synthetic; this is native-plan plus real-SQL component evidence, not a native
world authority capture, original parent CCM1 authentication or gameplay journey.
No active epoch is installed.

There are nine captures per engine: intact, old projection, nonnull receipt,
five schema-valid child faults, and restored. The exporter uses a separate
SELECT-only user; UPDATE is refused with error 1142. Every capture uses a
repeatable-read consistent read-only transaction, exactly one rollback and
cursor close, SELECT/SET/START queries only, and identical before/after seven-
table row inventories. Private owner mutations establish the faulty cuts
outside the read-only audit and are restored in `finally` blocks.

The five actual SQL faults are zero ID, wrong ID, wrong domain, wrong
discriminator and wrong parent. The old reader reports clean on all five per
engine; the repaired reader finds them at limits 0, 1 and 100. Invalidating the
first child also invalidates its descendant, so zero/wrong-ID cases correctly
produce two findings. The old four-field projection is clean for the preimage
reader but yields two missing-evidence findings now. A repeated child ID is
refused by the native SQL unique constraint with error 1062 and unchanged row
inventory; the separately corrupted copied export is also detected. These are
14 old-clean fault/projection cases across both engines.

The 60 recorded CLI checks cover both exception and operation views at all
three limits on the five SQL faults. Fifty-four return finding exit 1 with JSON;
six malformed-ID operation views correctly refuse with exit 2, empty stdout
and a safe stderr error. Input bytes stay unchanged. These refusals are tested
behavior, not skipped cases. A legitimate nonnull receipt ID projects exactly
and remains clean; the test does not claim receipt command/status validation.

### Resource budget

The budget cut contains 500 modeled roots, 32,000 derived children and 32,000
postings, at the native maximum of 64 children per root. Its actual encoded
facts occupy 11,876,227 bytes; ignored root metadata pads the file to the exact
32-MiB input ceiling (33,554,432 bytes). At limits 0, 1 and 100, the exception
view remains clean and leaves the file unchanged. Recorded wall times are
0.609501, 0.613640 and 0.681735 seconds; cumulative child peak RSS is 168,548 KiB.
All meet the existing 30-second/256-MiB component budgets. The RSS measure is a
conservative cumulative child maximum, not a release-host measurement.

### Retained failed harness attempts

The first native/SQL attempt stopped on two `unknown_opening` findings because
the harness omitted its modeled opening origins. The second expected only one
finding after a corrupted first child also invalidated its descendant. The
third attempted to parse JSON after a correct strict operation-view exit 2.
All three complete failed transcripts and fresh SQL/native artifact namespaces
are retained. The fixture and assertions were corrected; production refusals,
finding counts, budgets and schema constraints were not weakened. The final
fresh run passed both engines and all checks.

## Preservation, integration and notebook handoff

`tmp/plan5/child-identity-evidence.json` records 6,125 original regular tracked
inputs, all 1,498 native/migration inputs, original owned preimages, final owned
hashes, previous evidence hashes, and this slice's artifact hashes. The recorder
verifies all 49,603 prior artifacts, every unowned input, and final source hashes
again after the preservation pass. Earlier sealed namespaces are untouched.
The sealed manifest SHA-256 is
`9ca7d86f8724aba0fe001180f0ad5a20a764b42ddee6868f2e50e03436a550e8`;
it records 3,141 new artifacts and successful preservation of all 49,603 prior
artifacts and 1,498 native/migration inputs. The slice delivery repeats these
values alongside the result and verified remote commit SHAs.

The primary can integrate this single owned fix without an additive migration.
The narrow owned snapshot interface adds `domain_id`, `discriminator`,
`relationship`, and nullable `receipt_operation_id` to each child. Its consumers
are the independent reconciler and existing ID-only operator view; older exports
with children are explicitly unverified. The pure, native and both-engine tests
above establish the exact field/type/derivation/nullability invariants.

The primary must register new owner `test_plan5_child_identity.py` in
`tests/regression_manifest.json` and its explicitly gated workloads in
`tests/integration_manifest.json`. The central inventory's owner lookup must
recognize the file; no coordinator file is independently edited here. Keep
`profile`, `mode`, `minimum_cases`, resource reservations, `purpose`, and any
`manual`/`reason` metadata honest about the opt-ins. Select the eight pure
methods, the one native/private-SQL method, and the one budget method with their
separate class selectors, fresh directories, baseline preimages and exact
environment. Require exit 0 and zero skips for each selected workload. A default
whole-module run deliberately skips the two opt-in classes and must not be
advertised as the native/database qualification. The current primary's
registration policy and required-owner matrix check remain authoritative; do
not change their skip refusal or label these tests actual producer journeys.

For the primary's required notebook curator workflow, record this scoped defect
and repair, the exact candidate/manifest/delivery hashes, and the retained failed
attempts. Preserve the primary's local notebook as authoritative. Its local
location is not a blocker for this independent work.

Original complete parent-plan/detail and child-receipt authentication, native
capture authority, actual SQL/flat producers and gameplay, guarded publication/
recovery ACK, the coherent pending migration chain, current combined builds and
journeys, all writer completion evidence, and R1–R8 release-host latency,
storage-growth, checkpoint, retention and replica acceptance remain open.
Inactive behavior, wallet-root exclusions and the declined inactive spell-path
change remain preserved. No activation, production mutation, auto-correction,
deployment, merge or push to `experimental-accounting` occurs in this slice.

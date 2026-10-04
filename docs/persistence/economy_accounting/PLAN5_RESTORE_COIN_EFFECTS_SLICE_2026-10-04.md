# Plan 5: native coin witnesses in independent SQL restore qualification

This slice is based on `7a78bb065a7979b8dc8ad2ec49295629d6713906` in
branch `codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
The result commit and exact committed blobs are bound in the local manifest
`tmp/plan5/restore-coins-evidence.json`. Publish this lane to its own remote
branch; integration into `experimental-accounting` belongs to the primary owner.

The refreshed canonical remote is
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. All 1,232 native C/C++ inputs and
the migration inputs match the preceding qualified slice. The native source
tree remains `d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, from canonical base
`7d2f8e8153f637c38e19054cf1202436d3f9a28a`. The disposable schemas use the
canonical bootstrap and migration runner through `0056_spell_ward_durability`.
No result at 0055 qualifies this slice or the primary owner's combined candidate.

## Established defect and scoped fix

Native `economic_coin_effects_validate` gives ordinary accounts, kinds 1–6
and 11, denomination vectors and revisions. System accounts, kinds 7–10,
must have zero before/after vectors and zero before/after revisions. Their
postings record issuance, sinks, opening value and restitution. The system
witness is not a spendable native balance.

The base SQL restore qualifier applied `after - before = posting delta` to
every account. Consequently it rejected a valid native opening of seven
copper and accepted a fabricated system balance that satisfied that equation.
The earlier synthetic restore fixture used that fabricated system witness.

The RED run uses the base qualifier as an explicit read-only file mount, with
the final native fixture and disposable runner. On both MariaDB and MySQL,
canonical migration 0056 admits the intact native opening, but the base
qualifier refuses it with `restore_economic_account_delta_mismatch`.
The run has two failed engine subtests in 262.216 seconds and no skips.

The independent SELECT-only coin check now:

- Validates the existing 40-byte native account key, including its root
  lineage, version, kind, nonzero authority ID and zero padding.
- Checks adjacent account keys in native numeric kind/authority/context
  order. It does not sort little-endian hexadecimal text; UINT64_MAX remains
  representable with DECIMAL arithmetic.
- Preserves dense posting events, signed denomination weights and root
  balance, and refuses all-zero posting vectors.
- Requires ordinary before/after holdings to be nonnegative, with weighted
  totals at most INT64_MAX and valid revision advancement. Only ordinary
  accounts must reconcile their vectors to aggregate postings.
- Requires all system vectors and revisions to be zero, with a posting
  referencing each system effect. An unposted ordinary effect is admitted
  only when its vector is unchanged and its revision increases.

Existing orphan, count, index, account-reference, source, child, receipt and
item-link checks remain in the full restore qualifier. AST comparison also
requires all currency/epic history functions and `main` to match the base.
The new helper contains no mutation or repair path. SQL schema, native
contracts, shared coordinator, producers, registry/matrix and activation
owner are unchanged. No shared interface request is required for this slice.

## Native and canonical database evidence

`restore_coin_effects_fixture.cpp` encodes and decodes an opening EAI1/EAP1,
then two compound plans with a derived child and UID witnesses. The compound
plans exercise revision 1→2 and UINT64_MAX−1→UINT64_MAX. A rejected gambling
intent is also encoded. These are structural frozen fixtures; their command
binding and domain digest are not evidence of a real gameplay command.

The same fixture asks native coin validation for 32 decisions: all 11 account
kinds, the opening, invalid system witnesses, ordinary negative/overflowed
holdings and invalid revisions, revision-only effects, unposted effects,
zero/sparse/duplicate postings, invalid key version/padding and key ordering
at 255/256 and the uint64 boundary. Fifteen decisions are accepted. SQL and
`__NO_MYSQL__` modes emit exactly identical binary and framed output hashes
under C++20, strict warnings, ASan and UBSan:

- Binary: `1785d5b5e74f5f2011bbc42c9e1007bb59aa57e2d9a1530bd04ed7d2f59ea5e7`.
- Framed output: `0196c2e6489287091dbe742e10dc6883b5208b79f58f267fae66139714ea5b54`.

For each engine, the maintained runner creates a fresh schema and a SELECT-only
reader. An attempted UPDATE is denied with 1142. Its existing 26 corruption
cases still exercise the real full qualifier and unchanged native money
checks. Canonical intent/plan bytes and their actual hashes replace the
all-zero blob placeholder when the native fixture is supplied. The compound
and maximum-revision phases use their matching native plan bytes.

For the decision corpus, the disposable fixture owner commits each controlled
cut before the reader starts a read-only repeatable-read consistent snapshot.
Thirty cases reach the independent coin helper and must match the native
boolean. Canonical CHECK/UNIQUE constraints refuse the two sparse/duplicate
event insertions. All 15 captured tables are equivalent before/after each
audit, and the owner reconstructs the original fixture between cases. That
reconstruction belongs exclusively to this disposable test owner; the reader
never changes a finding or restored authority.

The corpus isolates coin semantics. It does not invoke the full qualifier
against substituted arrays that no longer match the retained canonical plan.
The checker still does not decode and bind canonical EAI1/EAP1 to normalized
SQL evidence. That separate, established integrity gap remains open.

## Exact tested source and commands

Frozen executable SHA-256 values:

| File | SHA-256 |
| --- | --- |
| `scripts/qualify_database_restore.py` | `31ec7d4770cb53ff67988cc35655b0e7026d31dc98ccdd5685d4f6eb77f65ced` |
| `tests/async/run_restore_accounting_evidence_mysql.py` | `2cfde7a6a833353a112afb0dac856a8e4297c2f61030dd934160de73effcfc3b` |
| `tests/async/test_restore_economic_coin_effects.py` | `a9c364eff043df0b1cb42fcea4a012b80eadffccec2cec578a2a67bbace318a6` |
| `tests/async/restore_coin_effects_fixture.cpp` | `fc7dddd26306c5c0613c6ab69f73a489e90b3e00947a85bd8ed91df1e5ff62a8` |

The immutable Docker image is `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
It contains Ubuntu 24.04, Python 3.12.3, GCC 13.3, MariaDB
10.11.14-0ubuntu0.24.04.1, MySQL 8.0.46-0ubuntu0.22.04.4 and PyMySQL
1.0.2-2ubuntu1.1. Workspace mounts are read-only, with only `bin/` writable.
Database authority lives in fresh Linux temporary directories, uses private
Unix sockets and `--skip-networking`, and receives a clean environment.
No project `.env`, production database or published port is used.

```text
docker run --rm --name duris-plan5-restore-coins-green2
  --mount type=bind,source=<worktree>,target=/workspace,readonly
  --mount type=bind,source=<worktree>\bin,target=/workspace/bin
  --workdir /workspace --env DURIS_RUN_RESTORE_COIN_INTEGRATION=1
  duris-plan5-origin-sql-tools:local
  python3 -u tests/async/test_restore_economic_coin_effects.py -v

# In the same image at /workspace/tests/async:
python3 -m unittest test_persistence_backup test_backup_review_remediations
  test_immutable_migration_runner test_backup_pfiles -v

# In the same image at /workspace:
make -C src -j2

# Host Python / WSL clang-format:
python -m py_compile scripts/qualify_database_restore.py
  tests/async/run_restore_accounting_evidence_mysql.py
  tests/async/test_restore_economic_coin_effects.py
scripts/format.sh --check --file tests/async/restore_coin_effects_fixture.cpp
git diff --check
```

Integration result: PASS, one guarded integration test with zero skips in
648.139 seconds. Both canonical engines pass all 26 existing corruption
refusals and all 32 native decision cases per engine: 30 audited decisions
and two canonical constraint refusals. Both native macro modes agree exactly.
The focused backup/restore/migration suites pass 83 tests with zero skips in
86.070 seconds. The server build passes as a no-op; its SQL executable hash
remains `ac7646a258aa5ebb5a05a95c51f93797ad95e97d74716b3b5a05042910425ace`.
Compilation and changed-file format checks pass. The Windows invocation of
the guarded integration test explicitly skips one test; it contributes no
database qualification.

The first setup attempt failed because the private root password was empty;
it establishes no accounting defect. The first corrected-qualifier attempt
timed out in its test wrapper at 240 seconds per engine. Both sources,
frozen hashes, logs and native artifacts remain preserved. The final wrapper
allows 900 seconds per engine and writes each engine log as output arrives,
so a timeout cannot leave stale diagnostics masquerading as a current result.
No qualifier assertion, database constraint or corpus expectation was relaxed.

Raw logs, frozen source hashes, the base qualifier and prior attempt inputs
are under `tmp/plan5/restore-coins-*`. Final native artifacts and engine logs
are under `bin/tests/plan5-sql-coin-effects/`; setup, RED and timeout artifacts
have separate preserved directories. The evidence recorder checks the union
of preceding slice artifacts, all native inputs and migration hashes, the
immutable image, exact frozen sources, native bytes, engine results and
committed owned/native Git blobs. Logs and binaries remain ignored local
evidence, not repository data.

## Integration action and remaining gates

Integrate this commit into the primary owner's completed local fixes and
incoming canonical migration 0056, then qualify that exact combined source.
This independent slice confers no release completion or activation authority.
Wallet-root item exclusions, inactive behavior, the declined inactive spell
change and active blackjack refusal remain preserved.

The following gates remain open:

- SQL restore canonical intent/plan decoding, digest verification and exact
  normalized evidence binding; complete native capture authority binding.
- The prior narrow shared handoffs for retained snapshot metadata and an
  authoritative per-epoch baseline marker, including the missing empty-book
  namespace case. Those remain primary-owner decisions.
- Executable evidence for every supported writer and real nonzero economic,
  UID, gameplay, fault/replay, lost-reply and publication journeys on both
  backends; populated upgrades on both canonical engines.
- Complete flatfile native capture, fair resumable historical sweeps and
  release-host mixed-workload p95/p99, growth, checkpoint/restart and audit
  budgets. Operation-ID order is not a commit watermark.
- Owner governance/export/typed-erasure decisions. Operator corrections and
  restitution remain disabled; financial history remains retained pending an
  approved retention horizon.
- The exact primary-owned combined candidate and its release qualification.

The required notebook curator workflow remains an external dependency.
`AI_CONTEXT.md` is absent from both this worktree and the original checkout.
Bounded accessible Pages searches for Duris and its notebook returned no
target; this does not establish that no notebook exists. The prior curator
request remains unanswered. This repository report is a handoff and does not
claim to have updated the project notebook.

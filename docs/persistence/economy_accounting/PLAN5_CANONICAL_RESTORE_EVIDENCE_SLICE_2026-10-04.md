# Plan 5: independent canonical SQL restore evidence

This slice is based on `fcdb1afd8a31a8ded500300a4530a115d9976bbd` in branch
`codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
The result commit and exact committed blobs are bound in the local manifest
`tmp/plan5/canonical-restore-evidence.json`. Publication uses this lane's own
remote branch. The primary owner integrates the slice into `experimental-accounting`.

The refreshed canonical remote is
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. All 1,232 native C/C++ inputs and
all migration inputs match the preceding qualified slice. Native tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b` comes from canonical base
`7d2f8e8153f637c38e19054cf1202436d3f9a28a`. Both disposable schemas use the
maintained bootstrap and immutable migration runner through
`0056_spell_ward_durability`. Earlier 0055 results do not qualify this slice
or the primary owner's combined candidate.

## Established defect and scoped fix

The base restore qualifier checked blob lengths and SQL evidence relationships
without independently decoding canonical EAI1/EAP1, verifying their hashes or
binding their contents to retained projections. Corrupting a retained intent's
leading EAI1 magic byte while preserving its length still passed the actual
full qualifier on both canonical engines. The controlled RED run preserves
the base qualifier verbatim and fails both engine subtests because corrupt
evidence was admitted. It ran one
guarded test in 368.154 seconds, with two failures and no skips.

`scripts/economic_restore_evidence.py` now interprets the existing version-1
wire contract independently using Python's standard library and scalar helpers
from the independent reconciler. It imports no native mutation codec,
coordinator, storage or producer implementation. Its SQL helper issues SELECT
only; it contains no correction or mutation path.

For every retained operation, the independent reader:

- Checks EAI1 size, header, zero padding, facts envelope, metadata, source
  grammar/policy, required original/source identities and nonzero command/domain
  digests. It verifies the tagged intent digest and persisted domain digest.
- Checks EAP1 header, zero padding, exact bounded counts and record widths;
  verifies the raw plan digest, intent/domain digests and complete metadata
  equality with EAI1 and the SQL root.
- Checks native numeric account-key order, ordinary/system vector semantics,
  revisions, dense balanced postings, reason/account policy and gambling shapes.
- Derives every child identity from its parent/domain/discriminator and checks
  native ready-child order. It validates UID order, owner/state/slot grammar,
  witness forests and item-event replay from the before to the after witnesses.
- Binds account, posting, child and item-reference projections exactly to the
  canonical records. It binds explicitly referenced custody ledger rows to
  UID/root/parent, owner/context, equipment slots and resulting item revision.
- Requires rejected roots to retain a valid intent, nonzero result code, no
  plan/digest and zero counts. Existing full-qualifier checks still reject extra
  normalized rows, orphans, source/receipt disagreement and incorrect links.

Custody owner aggregate revisions are separate native counters. The reader
does not compare them to item revisions. The disposable fixture corrects its
old room owner enum from 2 to native room enum 3 and proves that an independent
owner aggregate counter value remains admissible.

The reader retrieves capsules in 65,536-byte HEX chunks within the existing
client output bound, with an 8,192-byte EAI1 bound and 4 MiB EAP1 input bound.
It enumerates a quiescent restore candidate in 256-ID pages. This pagination
does not establish a live commit watermark or a fair resumable historical sweep.
The caller owns its read transaction; the full qualifier's existing main flow
is unchanged.

Only the independent restore module, its qualifier hook, the owned native
fixture/guarded runner and this report change. All other qualifier function ASTs,
including the coin helper and `main`, match the base. Native shared contracts,
coordinators, producers, schema, writer registry/matrix and activation owner
are unchanged. No shared interface change is required for this slice.

## Native and canonical database evidence

The expanded fixture uses actual native EAI1/EAP1 encoding/decoding in SQL and
`__NO_MYSQL__` builds, with C++20 strict warnings, ASan and UBSan. Its 3,026
decoder decisions cover two native intents and three native plans, byte flips,
truncation/trailing bytes, all 46 reason metadata decisions, opening account
policies, valid reason-specific plan shapes including gambling, a native
8,192-byte intent and a native no-source zero-effect intent/plan. The independent
decoder agrees with all native booleans: 1,054 accepted and 1,972 refused.
Both native macro modes emit identical bytes:

- Binary SHA-256: `ce7f1f48a5e241c446e9adb5adfa4c2a8b998180ef76db7091cad5fb162b2df8`.
- Decoder JSONL SHA-256: `c2c3954d21d8a56defe59447b844af1a412670f506f4f03d3b0670f0ba526a71`.
- Existing 37-block framed output SHA-256, unchanged:
  `0196c2e6489287091dbe742e10dc6883b5208b79f58f267fae66139714ea5b54`.

These structural capsules have frozen command-binding/domain values. Decoder
agreement is not evidence of actual gameplay commands or complete producer
capture. The existing 32 native coin-effect decisions remain byte-identical.

Each canonical engine runs the existing 26 corruption refusals through the real
full qualifier, then the new full-entry intent-magic refusal and repair. Its
additional 39 canonical cuts consist of 36 refused corruptions and three allowed
controls. Six cuts invoke the actual full qualifier; the remaining cuts isolate
the new helper inside the SELECT-only reader's repeatable-read consistent snapshot.

The cuts cover all three stored digests, rejected intent corruption, persisted
metadata, all six counts, account identity/revision, posting amount/child,
child domain/discriminator, item revision and seven custody fields. Truncated,
trailing and 4 MiB padded plan inputs are refused even when their stored plan
digests are recomputed. The 4 MiB test proves bounded malformed-input handling,
not a valid maximum-sized plan or release workload budget.

The balanced forgery changes a wallet after-value and both balancing postings
from seven to eight copper. Legacy conservation checks remain satisfied, but
the actual full qualifier now refuses disagreement with the retained plan.
The allowed controls are an owner aggregate counter, the native 8,192-byte
intent paired with matching plan hashes, and 259 retained roots spanning two
ID pages. Corruption of the last paged root is detected by the full qualifier.

The private fixture owner commits each controlled cut, then the independent
reader observes it. All 15 captured tables are identical before/after each
audit, including refused findings; owner reconstruction restores the original
fixture between cuts. The SELECT-only reader's attempted UPDATE is denied
with error 1142. Reconstruction is private test-owner work, never audit repair.

After canonical cuts, the prior 32-decision native coin corpus still passes on
each engine: 30 audited decisions and two canonical CHECK/UNIQUE constraint
refusals. This component check deliberately isolates coin semantics rather
than invoking the full qualifier against substituted projections that no
longer match their retained canonical plan.

## Exact tested source and commands

Frozen executable SHA-256 values:

| File | SHA-256 |
| --- | --- |
| `scripts/qualify_database_restore.py` | `7eb4c2426a0250b694ecde7ee888ad3cb0c6bb4a1f384f223674350f14baed1c` |
| `scripts/economic_restore_evidence.py` | `8c2ed0cdc44e5015a5c8a9f1485c008c77da705cc6710c70e86c6f7a61110ee4` |
| `tests/async/run_restore_accounting_evidence_mysql.py` | `e86096d0369687d92ee01bf80c983477601b47a3c6395d40a1810cbac023a456` |
| `tests/async/test_restore_economic_coin_effects.py` | `1ecebcf428cc1639847aaafe42eb25708f2cffeb75536fac00da755947302ab9` |
| `tests/async/restore_coin_effects_fixture.cpp` | `1ca5556a7c1c0c621d12e6fc163de793da0a57e523e8c336349348f96e1a5d84` |

The immutable image is `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
It contains Ubuntu 24.04, Python 3.12.3, GCC 13.3, PyMySQL 1.0.2-2ubuntu1.1,
MariaDB 10.11.14-0ubuntu0.24.04.1 and MySQL 8.0.46-0ubuntu0.22.04.4.
The workspace mount is read-only and only `bin/` is writable. Fresh database
authority lives in private Linux temporary directories and Unix sockets with
networking disabled. A clean private environment uses no project `.env`,
production authority or published port.

```text
docker run --rm --name duris-plan5-canonical-green2
  --mount type=bind,source=<worktree>,target=/workspace,readonly
  --mount type=bind,source=<worktree>\bin,target=/workspace/bin
  --workdir /workspace --env DURIS_RUN_RESTORE_COIN_INTEGRATION=1
  --env DURIS_PLAN5_CANONICAL_EVIDENCE=1
  duris-plan5-origin-sql-tools:local
  python3 -u tests/async/test_restore_economic_coin_effects.py -v

# In the same image at /workspace/tests/async:
python3 -m unittest test_persistence_backup test_backup_review_remediations
  test_immutable_migration_runner test_backup_pfiles -v

# In the same image at /workspace:
make -C src -j2

# Host Python / WSL clang-format 14:
python -m py_compile scripts/qualify_database_restore.py
  scripts/economic_restore_evidence.py
  tests/async/run_restore_accounting_evidence_mysql.py
  tests/async/test_restore_economic_coin_effects.py
scripts/format.sh --check --file tests/async/restore_coin_effects_fixture.cpp
git diff --check
```

Integration result: PASS, one guarded integration test, zero skips,
1024.896 seconds. Both canonical engines pass the full corruption and
canonical-cut matrix, followed by native coin parity. The focused backup,
restore and migration suites pass 83 tests with zero skips in 105.998 seconds.
The server build passes as a no-op, with unchanged SQL executable SHA-256
`ac7646a258aa5ebb5a05a95c51f93797ad95e97d74716b3b5a05042910425ace`.
Changed-file formatting, Python compilation and diff checks pass. The guarded
Windows invocation explicitly skips one test and contributes no database proof.

The first expanded native build failed for a missing `<algorithm>` include.
After repair, a focused run passed 3,022 native decisions and the initial full
corruption control on both engines in 270.701 seconds; it is separately frozen
and is not the final corpus. Four native boundary decisions were then added.
The first full run passed all 3,026 decisions, existing full-qualifier cases and
initial corruption control, then failed on both engines because the test
adapter returned only the first projection row. That failed run took 1,071.267
seconds. The adapter now returns every single-column row, matching the actual
SQL executor. The final run reruns the entire matrix. No audit assertion,
constraint, decoder decision or expected refusal was relaxed.

`tmp/plan5/canonical-*` retains base RED, compile, focused and adapter attempts,
their frozen source copies, hashes and logs. Final native bytes/engine logs are
under `bin/tests/plan5-sql-canonical-evidence/`; RED, focused and adapter attempts
have separate preserved directories. The recorder verifies the union of all
preceding slice evidence, unchanged native/migration/helper inputs, exact
attempt/final sources, native bytes, engine results and committed owned/native
Git blobs. Logs, private fixtures and binaries remain ignored local evidence.
The runner now requires an actual native fixture; the old fake zero-capsule
fallback is removed from this test path.

## Integration action and remaining gates

Integrate this commit with the primary owner's completed local fixes and
canonical migration 0056, then qualify that exact combined source. The present
slice qualifies independent structural SQL restore binding. It confers no
release completion or activation authority. Accounting inactive behavior,
wallet-root item exclusions, declined inactive spell change and active blackjack
refusal remain preserved.

The following gates remain open:

- Complete native capture and command/facts authority binding. SQL inbox
  `command_hash` hashes actual CCM1 bytes, while EAI1 command binding hashes a
  tagged normalized projection. They are different commitments. The inbox
  does not retain the full command preimage; this reader does not infer it or
  compare those different hashes. Any needed shared interface change belongs
  to the primary owner and requires a separate exact-field handoff.
- Native baseline/book/source-claim audit parity on an actual baseline-owner
  journey. The native SQL baseline writer persists one source claim, while
  the independent reconciler currently rejects reason-38 claims. The frozen
  opening capsules do not qualify native baseline witness/reservation authority.
- The existing narrow primary-owned handoffs for complete retained snapshot
  metadata and an authoritative per-epoch baseline marker, including missing
  empty-book namespaces and explicit old-artifact compatibility.
- Executable evidence for every supported writer, real nonzero economic/UID
  gameplay and fault/replay/lost-reply/publication journeys on both backends,
  and populated upgrades on both canonical engines.
- Complete native flatfile capture, fair resumable sweeps and release-host
  mixed-workload p95/p99, storage growth, checkpoint/restart and audit budgets.
- Owner governance/export/typed erasure and retention horizon decisions.
  Financial evidence stays retained; corrections and restitution stay disabled.
- Release qualification of the exact primary-owned combined candidate.

`AI_CONTEXT.md` and the required curator workflow/notebook reference remain
unavailable. The pending request has no answer; bounded accessible searches
returned no target and do not prove the notebook is absent. This report and
evidence manifest are curator-ready handoffs, not a claimed notebook update.
No accounting activation, production access/mutation, deployment, PR merge
or auto-correction occurs.

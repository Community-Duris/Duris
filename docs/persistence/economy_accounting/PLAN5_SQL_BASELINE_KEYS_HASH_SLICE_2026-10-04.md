# Plan 5: independently verify the existing baseline inbox fence digest

This slice uses branch `codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`, base
`79f07d75fc226056d35d7ecefce0025a70c60433`. It refreshes the primary publication
`65683a4b0a54e36d72b79440aa1a9437997f6f18`. Native source is frozen at tree
`c1dbd3e70f23548a23e9b7722046fc31f498e58c`; migrations at
`1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical 0056.
The result is the commit containing this report; its exact parent and diff
bind the separately solved issue. This lane publishes only its own branch.

## Defect and scoped complete fix

Every native baseline command owns one existing system-fence key, entity type
9, identity `0x45434f4e42415345`. The critical inbox already retains `keys_hash`.
Production hashing in `critical_command_repository.c` hashes the concatenation
of one type byte and eight little-endian identity bytes for each key. The
baseline therefore has a known nine-byte preimage independent of its original
admission timestamp.

The independent EAB1 reader checked terminal inbox status, result, revision,
type/schema/payload and the normalized EAI1 command binding, but did not select
or verify the existing inbox key digest. A baseline could retain arbitrary
inbox fencing identity while passing opening/root validation. This is separate
from the original admission-time gap; verifying the known fence is possible
before the primary's additive 0058 timestamp work is integrated.

The focused unmodified-reader test produces 43 failing refusal cases: all 32
digest-byte mutations, missing/NULL/empty/wrong-length values, zero digest,
foreign key type or identity, big-endian identity, the padded sixteen-byte CCM1
key representation, and a count-prefixed representation. The nine-byte inbox
hash input differs from the padded key representation used in CCM1 serialization.

The final reader selects the existing `i.keys_hash` as `inbox_keys_hash` and
requires exact bytes equal to `SHA256(type9 || little_endian(system_fence))`.
It reuses the existing committed-root refusal. Selected origins, retained
baseline source claims and both restore consumers share this independent
verification path. No mutation logic is imported; no authority is changed.

Owned files are:

- `scripts/economic_sql_audit_origins.py`
- `scripts/economic_restore_evidence.py`
- `tests/async/test_economic_sql_audit_origins.py`
- `tests/async/test_native_sql_baseline_audit.py`
- `tests/async/run_native_sql_baseline_audit.py`
- `tests/async/run_economic_sql_audit_snapshot_mysql.py`
- This report.

The synthetic sibling fixture now includes and seeds the already-authoritative
inbox column. No authoritative migration, native producer, shared contract,
registry, matrix, coordinator or activation file changes. There is no new
shared interface request from this slice.

## Qualification and evidence

The unmodified-reader native RED completes on MySQL 8.0.46 and MariaDB 10.11.14
through canonical 0056. Five wrong digests on each selected and retained book
are admitted by all three consumers on each engine: 20 damage cases and 60
consumer admissions. Exact before/after table rows are unchanged. Original
native digest is `bb010272361e10988fbb9333213b14e9fb974f840de82e834a9766b53ee33c3d`.
The RED harness exits 0 because it explicitly verifies this pre-fix gap;
elapsed container time is 381.491 s. The unit RED has 43 failing cases in 0.124 s.

After the first reader fix, all 25 focused origin/revision unit tests pass in
0.057 s, with zero skips. The first full GREEN attempt fails its intact baseline
control on both engines in 253.850 s (255.992 s including container startup).
The second restore consumer independently constructs its root row through a
JSON/hex SQL query. That query omitted the digest required by the updated pure
verifier. This failed run is retained; it establishes the additional consumer
update required by the complete fix. `economic_restore_evidence.py` now selects
the exact hex digest and decodes it as 32 bytes before calling the same verifier.
All call sites were enumerated; both paths are updated.

The final fresh GREEN completes on both engines: **157 damage cuts and seven
constraint checks per engine**, 314/14 total, with zero skips. All ten new
key-digest cuts per engine refuse through both restore gates. Selected epoch
capture raises `EAB1 committed root mismatch`; retained source-claim export
adds exactly one `baseline_source_claim` finding. Both restore consumers refuse
the corrupt baseline witness. The original partial snapshot counts are unchanged
on intact input: one `evidence_loss`, two `missing_native_holding` and two
`missing_native_item` findings. These tests do not promote the incomplete
world capture to complete evidence.

Final native unittest time is 566.684 s, 569.477 s including container startup.
The command is:

```sh
PYTHONPATH=/workspace/tests/async \
DURIS_RUN_NATIVE_BASELINE_AUDIT=1 \
DURIS_PLAN5_BASELINE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-baseline-sql-restore/keys-hash-green-final \
python3 -u -m unittest -v test_native_sql_baseline_audit.NativeBaselineAuditTests
```

RED additionally sets `DURIS_PLAN5_BASELINE_KEYS_HASH_RED=1` and uses fresh
`keys-hash-red` outputs. The failed initial GREEN uses fresh `keys-hash-green`;
all three stages are preserved. Each invocation requires a fresh directory
below `bin/tests/plan5-baseline-sql-restore`; the existing path guard refuses
overwriting prior runs. Python cache prefixes are fresh children of
`bin/tests/plan5-baseline-keys-hash`.

Other exact commands:

```sh
python3 -u -m unittest -v \
  test_economic_sql_audit_origins.OriginTests \
  test_economic_sql_audit_origins.ItemRevisionTests
python3 -m py_compile scripts/economic_sql_audit_origins.py \
  scripts/economic_restore_evidence.py \
  tests/async/test_economic_sql_audit_origins.py \
  tests/async/test_native_sql_baseline_audit.py \
  tests/async/run_native_sql_baseline_audit.py \
  tests/async/run_economic_sql_audit_snapshot_mysql.py
git diff --check
```

All pass. The focused RED command invokes only
`OriginTests.test_baseline_receipt_keys_hash_requires_exact_native_system_fence`
against the unchanged reader. RED and final test/helper source copies are
preserved; only the two independent reader query/verification paths change
between the native RED and final GREEN invocations.

Runs use immutable image `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with Python 3.12.3, GCC 13.3.0 and OpenSSL 3.0.13. Docker has `--network none`,
a read-only `/workspace` bind and a writable `bin` output bind. The recorder
also has an explicit writable `tmp/plan5` bind. SQL daemons use newly initialized
private Unix sockets, disposable-only credentials and no project `.env`.
MySQL is `8.0.46-0ubuntu0.22.04.4`; MariaDB is
`10.11.14-MariaDB-0ubuntu0.24.04.1`. Each native schema is fresh, applies the
canonical bootstrap and migration runner, and asserts terminal history
`56 / 0056_spell_ward_durability`. No 0055-only or synthetic sibling schema
result qualifies these canonical native databases.

The original production native baseline owner publishes both books, each in
its own epoch, with `active_epoch` NULL. The SELECT-only audit account denies
UPDATE. Every native damage cut compares exact rows across 18 source tables;
foreign-key enforcement remains enabled for reader and native replay calls.
Only the private fixture owner introduces and restores deliberate test damage.
Readers do not correct findings or mutate those rows.

Both repaired databases are dumped and imported into a second fresh private
daemon of their respective engine. Full history/evidence qualification passes,
all 18 table images match, and native `--reconcile` returns exactly the original
encoded result without changing authority. Cold dump evidence is:

| Engine | Dump bytes | SHA-256 |
| --- | ---: | --- |
| MariaDB | 332,977 | `04a92e392f2c6723d380ee03974cf6e5dab3fa31b8e6576f48e57472d7b14f73` |
| MySQL | 341,737 | `ce42badf313fb87cc623af7e3c7ff5f8ebee3557c5434a3f6b2bf6035fa4469d` |

The native fixture compiles in SQL and client-free modes with the existing
strict C++20 `-Wall -Wextra -Wpedantic -Werror -O1 -g` closure and
ASan/UBSan, `-fno-omit-frame-pointer`, `-fno-pie`, and `-no-pie`.
`DURIS_ECONOMIC_SQL_BASELINE_TEST` permits test access to the production owner;
the client-free build also defines `__NO_MYSQL__`. SQL links the existing
mysqlclient flags; both link libcrypto. Both execute with leak/error-halting
ASan and error-halting UBSan options, without sanitizer findings. The existing
13-source compile list and exact commands are preserved in the wrapper source
and native build metadata. No native mutation implementation changes.

All three stages have identical native fixture binary hashes:

- SQL: `0d7d7dca94aa58540d4a13f35ccdc545abf5b178f9cc7ba97e5752d151071fc0`.
- Client-free: `2d624696a8b7d4a6735f8b325f305fcbe5233541ec1935073e3c31bb3f77d874`.

The final source inventory confirms all 1,254 native files and 236 migration
files unchanged. All 26,567 protected prior artifacts remain byte-for-byte
unchanged. Local ignored evidence paths are:

- `bin/tests/plan5-baseline-keys-hash/`: immutable reader/test source copies,
  unit/native logs, timing/result records, syntax proof and preservation report.
- `bin/tests/plan5-baseline-sql-restore/keys-hash-{red,green,green-final}/`:
  native binaries/build metadata, engine logs, cold dumps and frozen input
  manifests. The middle stage is explicitly failed evidence.
- `tmp/plan5/baseline-keys-hash-base-tree.txt`: exact frozen native/migration
  Git entries.
- `tmp/plan5/baseline-keys-hash-evidence.json`: final source and artifact
  SHA-256 inventory of 697 fresh evidence files. Manifest digest is
  `d70f07a4bb04761e37b6ce6ff72850e5ecc61c81ee0ba2ec1c95bcab383b3b5e`.

## Remaining gates and curator handoff

The original full baseline CCM1 `command_hash` still requires its real original
admission time. The primary's confirmed nullable-for-historical
`economic_baseline_witness.command_accepted_at_usec` interface remains the
existing handoff. Native writes/replay and coherent additive 0058 migration
after pending0057 remain primary-owned. Independent complete command verification
and both-engine damage/cold restore must consume that actual implementation.
No timestamp is fabricated or inferred from SQL creation/commit times. This
known-key verification does not claim complete command preimage authentication.

The maintained full flatfile build on this native tree still has the signed
parent-index compiler finding in `coin_physical_recovery.c:1225`. The primary
owns that repair. The unchanged failing build is not repeated, and managed v3
service boots remain unexecuted. The native baseline owner closure in this slice
has its own exact source scope and does not qualify that full server build.
During final qualification the primary published the real signed-parent repair
as `8f75a8964d473a15ee3bcc7806593eb7eac48b5c`. That native change is not part
of this frozen keys-digest slice. The next maintained service qualification
can adopt it on a separate base with fresh evidence paths.

Inventory, synthetic fixture and isolated native proofs do not establish
release completion. Writer/producer journeys, combined source qualification,
world capture/install and all applicable R1–R8 gates remain open. Accounting
stays inactive, wallet-root item exclusions and the declined inactive spell
path behavior are preserved. No production access, deployment, activation,
PR merge or audit correction occurs.

The primary maintains the shared notebook locally through the curator workflow,
per the user's clarification. This report supplies the exact curator handoff;
notebook access is not a blocker. The primary alone integrates the separately
committed slice and publishes the tested combined candidate.

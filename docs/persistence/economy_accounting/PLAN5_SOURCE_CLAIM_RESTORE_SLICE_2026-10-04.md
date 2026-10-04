# Plan 5: exact retained source-claim restore qualification

Delivery branch: `codex/accounting-plan5`.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Slice base: `8f27cb505f8934bd634dcd3625ea02594c86a9a6`.
The result is the commit containing this report; the final handoff and ignored
`tmp/plan5/source-claim-evidence.json` record its exact SHA after commit.
Refreshed remote: `f7d26eaa721cd3b675c0b0c65009a2535813f400`. Native tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b` retains canonical migration 0056.
Primary-owner unpublished integration is outside this source qualification.

## Established defect and complete retained-claim fix

The actual native encoder staged four records in an inactive store: two
successful source-bearing plans in retained epochs 50 and 51, one rejection
with an unclaimed event, and one rejected retry referring to the first root's
event. Native `stage_source_claim` wrote exactly two claim files in the same
authority commits as their records. The actual existing native restore qualifier
accepted the intact store and still returned zero after one required claim was
deleted. The remaining economic bytes were identical. This executable defect
is retained in `tmp/plan5/flatfile-source-claim-loss-red.log` and its reproduction
driver. The preceding fixture setup failure is retained separately; it stopped
before restore qualification because the test attempted to freeze schema 2.
Refreezing through schema 1 corrects that fixture without changing native code.

The independent read-only operation reader now checks each successful nonbaseline
source-bearing record's exact `DURSCL1` frame. Its filename is the lowercase
SHA-256 of the 16-byte lineage and complete 48-byte source event. Its body is
exactly lineage, event, owning operation ID, successful outcome 1 and zero
reserved bytes. Private-file checks, complete frame bounds, version and checksum
use the existing independent frame reader. No native codec, storage getter,
writer, recovery or claim verifier is called by the audit reader.

The reader then sorts expected claim digests and checks every source-claim file
against that set, rejecting orphan or malformed filenames, duplicate successful
source events and unexpected file counts. Claim keys are lineage-wide, not
restricted to the latest epoch. Rejected receipts remain bounded opaque results
without plans or claims; they can refer to an event owned by another successful
root. A claim owned by a rejection refuses. Source-free successful roots are
unchanged, as are absent/empty and partial inactive installations.

Native baseline batches retain dedupe in their own EAB1 witness/reservation book
and do not stage common source-claim files. A native compatibility probe caught
false refusal by the first candidate, before commit. The final reader recognizes
the existing baseline contract: command type 20, reason 38, writer 4, operator
actor, source kind 10, generation equal to epoch, slot 0, zero original operation
and the 48-byte EBC1 command. This preserves the native baseline seam; complete
independent book/witness qualification remains an open gate. A native-encoded
baseline positive and a rebound attempt to disguise a common source as baseline
cover this boundary. No native schema, interface or storage rule is changed.

The native 256-bucket, 4096-entry format bounds expected keys to 1,048,576
SHA-256 digests: 32 MiB of digest storage, with one index/segment retained at a
time. This records a structural bound, not a measured latency, memory or workload
qualification. No new operational limit, configuration or wire format is added.

## Owned files and shared handoff

- `scripts/qualify_flatfile_economic_records.h`: exact claim ownership and coverage.
- `tests/async/flatfile_restore_authority_fixture.cpp`: native source plans,
  claimless rejections and cross-epoch claims. It never selects an active epoch.
- `tests/async/test_flatfile_restore_economic_authority.py`: native qualifier,
  independent ASan/UBSan reader, corruption and unchanged-evidence checks.
- `tests/async/test_persistence_backup_integration.py`: expands the existing
  economic-file-loss method to capture and refuse both missing native claims.
- `docs/operations/BACKUPS.md` and this report: operator behavior and proof limits.

No shared fields, schema, contract, coordinator, writer or activation-owner
change is requested. The preceding primary-owner manifest handoff remains:
register the native script in `tests/regression_manifest.json`, raise the
recovery entry's `minimum_cases` from 10 to 11, and add
`PersistenceRecoveryIntegration.test_flatfile_economic_record_loss_refuses_before_service_boot`
to the integration entry's `required_cases`. This slice extends that same case;
the count remains 11. Shared manifests and primary-owned files are untouched.

## Exact source and validation

The ignored evidence manifest pins all 1,232 native inputs, owned executable
inputs, native encoder/qualifier source lists, unchanged build/restore helpers,
migrations, tools images and raw output hashes. Post-commit verification compares
committed bytes against tested inputs; final result and artifact pins are in the
manifest and final handoff.

| Executable input | SHA-256 |
| --- | --- |
| `scripts/qualify_flatfile_economic_records.h` | `e1a7fe089714cd3dfe58849f403c1957ed5dd1503c2414407e1db54c0f69ef66` |
| `tests/async/flatfile_restore_authority_fixture.cpp` | `b1a6625ab773d4a5400f559da57e7a4f2b54156edc5ea9b970724fd8b2d87f07` |
| `tests/async/test_flatfile_restore_economic_authority.py` | `adf70fe67241f787c1f036193663c40ca5d90693c058cfb46f650f9e90cda592` |
| `tests/async/test_persistence_backup_integration.py` | `ad75c774681f11d012fecb9a4e191c81efbbe01f037b5deb0fc9ee2adc432787` |

Native qualifier SHA-256:
`2a3b311760ff7ee160139ecf392c720d6f8cbb41ff59b2b3ba61fdbbdc41d8de`.
Native encoder fixture SHA-256:
`4481dc92bb3b4f155c41b27e59b1614dc482ef290033a9ace65094a04b957178`.
Independent sanitized reader SHA-256:
`0b63c1423202d7e4affee7998750b3dd475a412be138cc1f472877ba87827d9e`.

Focused tests use Ubuntu 24.04, Python 3.12.3, GCC 13.3.0 and
`duris-accounting-restore-tools:local`, image
`sha256:0232af0d2069d00424bfe14ae109224395050f46282896682913049de0b73b25`.
The checkout is mounted read-only at `/workspace`, with only this worktree's
`bin/` writable. Commands:

```sh
python3 tests/async/test_flatfile_restore_economic_authority.py
python3 tests/async/test_persistence_backup.py
make -C src -j2
make -C src PERSISTENCE_BACKEND=flatfile DMS_BINARY=/workspace/bin/server/dms_restore_flatfile -j2
```

The native regression includes intact cross-epoch claims and a claimless rejected
retry, missing claims from both epochs, exact metadata/outcome/reserved bytes,
frame corruption/size checks, malformed/orphan filenames, swapped claim bodies,
rehashed/rebound duplicate-source roots, a root that drops its source metadata,
claims owned by rejected receipts, and symlink/hardlink/FIFO/nonprivate files.
It retains all preceding authority and operation-store cases. Every case invokes
both native qualification phases and the independent reader; every invocation
checks retained bytes, permissions and link counts are unchanged. The reader
links only owned headers and libcrypto. Native encoders are also sanitized.
The final native suite passes 13 intact-store cases and 144 corruption refusals:
314 native qualifier invocations and 157 standalone sanitizer-reader invocations.
Backup filesystem tests pass 40 cases; both maintained development builds and
explicit formatter file checks, Python compilation and `git diff --check` pass.

Full recovery qualification uses the existing derived local tools image
`duris-plan5-mysql8-recovery-tools:local`, pinned to
`sha256:192535b64b6212f908bd54befe5ced5285494875d675a15202aee20c571f7058`,
with `CAP_SYS_ADMIN` and unconfined seccomp for isolated namespaces:

```sh
DURIS_RUN_BACKUP_INTEGRATION=1 DURIS_RUN_MYSQL_BACKUP_INTEGRATION=1 \
  python3 -u tests/async/test_persistence_backup_integration.py -v
```

The expanded manager method captures five damaged generations: missing index,
sealed segment, active segment, and each native claim. It verifies each manifest
and retained control, then requires refusal before service boot, no qualification
receipt, and unchanged source/generation inventories. Healthy copies first pass
actual native preflight. The final full suite passes all 11 cases with no skips
in 606.195 seconds. This is elapsed run evidence, not a measured workload budget.
The first full run passed the ten unchanged cases but
reported five fixture errors: the two new live paths were absent from the test's
configured `live_roots`. Capture correctly refused before the missing-file
assertions. The fixture now authorizes its exact selected disposable root; no
production policy is changed. The failed run and its original frozen inputs
remain separate from final qualification. Existing cases exercise native recovery,
WAL/quarantine,
receipts, private foreign-owned checkout isolation and actual SQL dump/import,
schema/history/value checks with isolated service boot on MySQL 8.0.46 and
MariaDB 10.11.14. Daemons use fresh private datadirs and Unix sockets, TCP and
native AIO disabled. Fresh bootstrap applies/adopts canonical migrations through
0056; upgraded populated installations remain unqualified.

Raw evidence under ignored `tmp/plan5/`:

- `flatfile-source-claim-loss-red.log`, `flatfile-source-claim-fixture-setup-failure.log`.
- `source-claim-baseline-compatibility-red.log`,
  `source-claim-recovery-fixture-policy-failure.log`, `source-claim-first-pass-inputs.json`.
- `flatfile-source-claim-native-green.log`, `flatfile-source-claim-backup-unit.log`.
- `source-claim-recovery-both-engines.log`, `source-claim-sql-build.log`,
  `source-claim-flatfile-build.log`, `source-claim-format-check.log`.
- `source-claim-frozen-inputs.json`, `source-claim-evidence.json` and recording driver.

## Remaining gates and blockers

This fixes retained source-deduplication evidence loss and ownership validation.
Logical source entitlement and producer authorization, complete account/posting
and item semantics, actual native holdings/custody/opening reconciliation, pile
heads, baseline witnesses, erasure and allocator/activation authority remain
required. Full retention/rotation horizons, measured budgets, populated upgrades,
real accounting gameplay/fault journeys, production-profile checks, writer-census
coverage and the primary owner's tested combined candidate remain open.
Native-encoded fixtures and recovery boots do not establish release completion.
Earlier 0055 evidence is not relabeled as qualification of this source.

No configured game, `.env`, production data, accounting activation, deployment,
PR merge or audit correction is involved. Wallet-root item exclusions, inactive
behavior and the declined inactive spell-path change remain untouched.

The curator notebook update still needs the missing `AI_CONTEXT.md` and required
workflow/notebook reference. The pending input request remains; this source report
does not substitute for or claim a curator notebook update.

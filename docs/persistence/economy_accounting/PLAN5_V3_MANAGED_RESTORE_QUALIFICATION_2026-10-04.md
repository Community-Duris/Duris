# Plan 5: maintained v3 managed restore qualification

Branch `codex/accounting-plan5`; worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Frozen tested base is `db95a9d8b2965a9e7bb355aa3469c9d4f6b89186`, a normal
merge of primary `8f75a8964d473a15ee3bcc7806593eb7eac48b5c` after independent
receipt-fence slice `3c8ba021a7dd8b3ac9048651672c9a456774c5d7`.
Native tree is `8352e470e9d32bee2fc84cb90097d0ecfb87ce2d`; migration tree is
`1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical 0056.
The report result SHA and verified remote branch SHA are supplied in delivery.

## Established build defects and adopted repairs

The earlier maintained attempts failed on the missing replay-ownership header
and the signed parent-index comparison. Both failures, their partial objects,
source fingerprints and diagnostic copies remain intact in their original
evidence namespaces. Primary owns and published both actual repairs. This run
adopts those commits through normal merges and executes the maintained build;
it does not independently modify any server, migration, coordinator, registry,
matrix, producer or shared accounting contract.

The signed comparison now excludes negative parent sentinels before conversion
to `size_t`. The actual adopted recovery-file SHA-256 is
`0670e71865ebe3543ec18c4142653ed0ea6499cd93cc3bb36f06407f951e8397`,
matching the primary's reviewed repair and the earlier copied diagnostic.
See the [original compiler handoff](PLAN5_V3_PHYSICAL_RECOVERY_BUILD_HANDOFF_2026-10-04.md)
and [primary repair](COIN_FLAT_PARENT_INDEX_REPAIR_2026-10-04.md).

Owned tracked file: this report only. No new shared interface is requested.
The pending original SQL baseline admission-time interface remains primary-owned
and is a separate qualification gate.

## Exact execution environment and commands

All commands execute in Docker image `duris-plan5-origin-sql-tools:local`,
immutable ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
It supplies Ubuntu 24.04.4, GCC 13.3.0, Python 3.12.3 and OpenSSL 3.0.13.
The source checkout is mounted read-only at `/workspace`, `bin/` is writable,
and networking is disabled. Recorder invocations alone also mount `tmp/plan5/`
writable. No `.env`, live database, Redis, external service or production state
is consumed. The managed test receives `SYS_ADMIN` and `seccomp=unconfined` for
its isolated user/network/PID namespaces and private tmpfs state.

Fresh maintained build, using its existing C++20 hardening and warning flags:

```sh
make -C src -j2 PERSISTENCE_BACKEND=flatfile \
  OBJDIR=/workspace/bin/tests/plan5-lifecycle-v3-repaired/objects-flatfile \
  DMS_BINARY=/workspace/bin/tests/plan5-lifecycle-v3-repaired/server-flatfile
```

The build passed, exit 0, in 504.4190718 seconds: 716 compilation commands,
716 objects, zero warning/error lines. No old object or server binary is reused.
The server SHA-256 is
`40f068347c35f46f5c350be96d31bb39b65dc95178c3e65a844103a70d9776eb`.

Managed test, with fresh Python cache and selected server/artifact paths:

```sh
PYTHONPATH=/workspace/tests/async \
DURIS_RUN_BACKUP_INTEGRATION=1 \
DURIS_PLAN5_LIFECYCLE_BACKUP_SERVER=/workspace/bin/tests/plan5-lifecycle-v3-repaired/server-flatfile \
DURIS_PLAN5_LIFECYCLE_BACKUP_ARTIFACTS=/workspace/bin/tests/plan5-lifecycle-v3-repaired/native \
python3 -u -m unittest -v \
  test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration
```

One test passed with zero skips: 312.515 seconds inside unittest,
316.4745636 seconds including container startup/exit. The class builds fresh
independent qualifier and state fixtures. The lifecycle fixture uses the existing
verified native build helper, reports `built` and zero reused objects, and has
C++20 strict warnings, ASan/UBSan and the existing native flatfile test seam.
Its actual executed cache path is recorded in `native/evidence.json`; an exact
hash-verified copy is retained as `native/lifecycle-fixture-executed-copy`.
Its compile/link/lookup times are 40.727/12.096/38.724 seconds. These helper
artifacts are distinguished from the maintained server's fresh object directory.

## Executed recovery and retention results

The production backup and restore managers run against a private copied tool and
runtime checkout, using modeled inactive native-codec lifecycle history with
known native origins. The fixture retains an old initialized epoch and a newer
inactive epoch. This run establishes:

- Managed capture preserves the original required `.elr` receipt and economic
  evidence bytes, mappings, native witnesses, retained baseline and source dedupe.
- Restore qualification and the first actual server boot replay the original
  native WAL and pending transaction. Player-save and critical-command journals
  drain; loaded state includes one account, one identity, one player and one
  snapshot.
- A second actual isolated server boot changes only the UID allocator and its
  sealed witness. `(next_uid, revision)` advances from `(1000203,2)` to
  `(2000203,3)`, exactly one maintained boot reservation, with valid checksums.
- Receipt bytes, source state, source journals and captured generation inventories
  remain unchanged. Accounting remains inactive.
- Pruning removes the unretained old generation and preserves the two required
  generations, including their old inactive receipt.
- Manifest-anchored receipt loss and corruption both refuse before any service
  boot. A checksum-valid semantically corrupt receipt also refuses before boot.
- A backup captured after the required receipt was already missing has a valid
  manifest but refuses before boot through native required-file discovery.

The final class report records `completed=true`,
`required_file_discovery_qualified=true`, two actual service boots, and preserved
source/runtime/helper inputs. `source_capture_executed`,
`lifecycle_install_executed`, `accounting_activated` and `full_R8_qualified` all
remain false. The known-origin discovery result is bounded to this modeled
history; it does not authenticate a real native source cut or installation.

## Source contracts and evidence

On the same frozen source, `python3 scripts/validate_economy_accounting.py`
passes in 12.949822 seconds: 14 fixtures, 886 routes and 2,843 candidate sites.
`python3 scripts/generate_economy_writer_coverage.py --check` passes in 12.200849
seconds with zero unmapped current sites. These are inventory/contract checks.
`python3 scripts/validate_economy_accounting.py --release` exits 1 in 0.215198
seconds with `writer has no executable evidence`; release remains BLOCKED.

Evidence root is `bin/tests/plan5-lifecycle-v3-repaired/`: full maintained build
log, command/timing and binary/716-object metrics; native command/log; class
source/runtime/helper fingerprints; first/second service logs; cold-restart state
delta; contract logs/results; copied recorder and driver; initial and final
preservation records; and the exact executed lifecycle-fixture copy/provenance.
Frozen Git entries are `tmp/plan5/lifecycle-v3-repaired-base-tree.txt`.
Final manifest is `tmp/plan5/lifecycle-v3-repaired-evidence.json`.
It verifies all 1,254 native and 236 migration inputs, all Python inputs under
`scripts/` and `tests/async/`, and all 27,264 protected prior evidence files
unchanged after the completed service test. It records 1,640 fresh evidence
files. Manifest SHA-256 is
`2ffac5d406b7c3584931399d90bbfa2ac51caa1be9d8ffee0f2afd840a032a36`.

## Remaining gates and curator handoff

This is maintained flatfile build and managed recovery proof on the exact base.
The SQL maintained build and disposable SQL checks are not run in this slice.
The separately published [receipt-fence qualification](PLAN5_SQL_BASELINE_KEYS_HASH_SLICE_2026-10-04.md)
remains pinned to its own native/schema 0056 source; it is not relabeled as SQL
qualification of this later candidate. Pending 0057/0058, the full original
baseline command preimage, real native source capture/install, actual retained
pile publication/recovery, producer player journeys, measured complete workloads,
writer coverage and combined release acceptance remain open.

Primary advanced to `2a357bdeb066f0acc30932bfb7dfa7ba5862edad` during this run.
That commit adds central native stake audit registration and a tools dependency;
it is fetched but not adopted during this frozen test. It changes no native or
migration input. Any adoption/result commit is reported separately.

Primary maintains the shared notebook locally through its curator workflow,
per the user's clarification. This report is the precise curator handoff;
notebook access is not a blocker. Accounting remains inactive; wallet-root item
exclusions and the declined inactive spell-path behavior are preserved. No
activation, audit auto-correction, production mutation, deployment, PR merge or
direct push to `experimental-accounting` occurs.

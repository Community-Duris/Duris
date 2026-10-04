# Plan 5: managed persistence of retained lifecycle receipts

Branch `codex/accounting-plan5`; worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Refreshed primary `46e37a1fc47ce5e8d5de82f4455fd1d048ae033f` is imported
by a normal history-preserving merge in slice base
`1e4ed8903c914d12c3d23c72ea304ffc632a29ae`, with peer parent
`9306818b5e3102aa692a35a5ec8a1b1a84d87efa`.
Native tree `939f53adc32e290e8b285f1ab606dd7df7dd02e5`; migration tree
`ee51e2c36250ffda10c01170555f383229cd731c`, including canonical0056.
The result SHA and remote readback are in the delivery and ignored local manifest.

## Qualification scope and ownership

The production backup already captures the complete economic-evidence directory.
This slice adds executed managed backup/restore/retention proof for a present
original `.elr`, including an old initialized epoch with a newer inactive epoch.
It makes no production implementation or schema change.

Owned files are `tests/async/test_persistence_backup_integration.py`,
`tests/async/persistence_restore_fixture.cpp` and this report. The existing
integration suite, backup/restore managers, native state/WAL and lifecycle-codec
fixtures, service namespace qualifier and policy are reused. Existing workspace
executables remain intact; all new tools have fresh explicit `bin/tests/` paths,
and the managers receive a private copied checkout for those tools/assets.

The native state fixture's existing interrupted transaction seeding is extracted
into one shared helper. Its new `seed-pending-transaction` mode invokes the same
native fault seam after the economic setup; original seed modes still invoke it.
The first executed attempt established that economic commits recover an earlier
pending transaction. Seeding the final interruption afterward preserves the
actual split after-image journal needed by this test. This is a fixture ordering
correction, not an established production defect.

The lifecycle fixture uses the actual native codec, generic authority/baseline
participants and common store, with original mappings/descriptors/witness/root.
Its sources and external cutover flags remain modeled. No lifecycle install,
live native source capture or active epoch selection is invoked. The original
receipt's exact bytes are preserved; healthy inactive behavior is exercised.

## Executed evidence

Validation uses Ubuntu24.04/Python3.12.3/GCC13.3.0/OpenSSL3.0.13 in image
`duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Source is read-only and only `bin/` is writable. Native service validation adds
`CAP_SYS_ADMIN` and an unconfined seccomp profile for the existing tmpfs/user/
network/PID namespace isolation. Docker network remains disabled; service
listeners run only in the newly created namespace. No runtime `.env`, existing
database, Redis service, production state or external transport is used.

```text
make -C src -j2 PERSISTENCE_BACKEND=flatfile \
  DMS_BINARY=/workspace/bin/server/plan5-lifecycle-backup-flat-dms_new
DURIS_RUN_BACKUP_INTEGRATION=1 \
  DURIS_PLAN5_LIFECYCLE_BACKUP_SERVER=/workspace/bin/server/plan5-lifecycle-backup-flat-dms_new \
  DURIS_PLAN5_LIFECYCLE_BACKUP_ARTIFACTS=/workspace/bin/tests/plan5-lifecycle-backup/fourth \
  PYTHONPATH=/workspace/tests/async \
  python3 -u -m unittest test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration
python3 tests/async/test_flatfile_backup_manifest.py
python3 tests/async/test_persistence_backup.py
python3 -m py_compile tests/async/test_persistence_backup_integration.py
./scripts/format.sh --check
git diff --check
```

The maintained flat-file build and its post-fixture incremental rerun pass.
Server SHA256
`d2d00f7bf41b6ab1567fef8d7eefa79a00347c282b73fc5edeb8793351455a63`.
Four existing manifest tests and40 existing backup policy/fault tests pass.
Native full-qualifier SHA256
`e897c15ba07bc42191ff79f824ab23b1f7be56b483958a6727904305fe69890d`;
state/WAL fixture
`da042a8d640e6e11d672d6e32718c23f6ec978007f7097211fd1b9e11841cc95`;
lifecycle fixture
`5aaf341cf1ccdb4553fa603fb8b0b77b66c4d274e8d4f1b0f6cfbde84aaf4984`.
Formatting uses WSL clang-format14 and explicit managed-worktree Git paths.
The complete integrated run passes in318.871 seconds, including its three
native tool builds, with one executable journey and zero skips. Both real service
boots and all three before-boot refusal cases pass; native source and every
consumed fixture/tool/runtime input remain frozen across the run. The preserved
first/second service logs confirm readiness and normal shutdown. This is actual
native service/replay evidence on mini runtime assets, with modeled economic
source descriptors, rather than a real player gameplay or full-world census.

The integrated fixture captures native player/critical WAL and an actually
interrupted domain transaction together with the original lifecycle receipt.
Managed restore must recover both after-images, load the native account/player,
drain journals and boot/shut down the real maintained server. A second cold boot
uses the same recovered candidate and copied runtime. Native verification after
both boots requires original critical-operation dedupe and the expected player
revision/status; independent audits preserve old economic evidence and verify
active epoch remains zero.

Cold boot correctly advances the native item UID reservation. The first whole-
state equality oracle was too broad: each boot reserves1,000,000 IDs. Its observed
allocator SHA prefixes `83a1238c...` and `71f1fba8...` exactly match the native
wire encoding of `(next_uid,revision)` advancing from `(1000203,2)` to
`(2000203,3)`. The corrected test validates both allocator/checksum and sealed
high-water witness, permits exactly those two changed files, and requires every
other state file and all economic evidence to remain identical. This preserves
the non-reuse invariant rather than suppressing the native boot behavior.

Transport damage is tested on isolated copies of the authentic captured manifest:
complete `.elr` loss and corruption must refuse before service boot and leave the
damaged generation and intact source unchanged. Separately, a present receipt is
deliberately corrupted in the disposable source with a valid outer checksum,
then captured by the real manager. Valid transport checksums must not bypass the
independent semantic preflight; no service boot or automatic correction follows.

Generation retention uses the existing test policy and simulated timestamps,
not a new operational retention duration. Two old generations plus a current
generation exercise actual pruning of only the unretained oldest generation;
both retained generations must keep the original economic files byte-for-byte.
Source journals and the source authority remain unchanged through capture and
restore, except for the explicitly modeled later source-corruption fault.

Evidence root `bin/tests/plan5-lifecycle-backup/` contains all build/check logs,
the completed run's source/runtime/binary fingerprints, both actual service logs
and `cold-restart-state-delta.json`. All generated data/logs remain outside Git.
An initial pre-execution run was stopped to add a missing Python import; the
ordering failure and original restart-oracle failure remain preserved in the
second/third runs. They are not semantic production REDs or completed acceptance.
Original boot readiness60-second and service120-second bounds are preserved.

## Remaining gates

This proves managed persistence of the **present modeled receipt** at these
specific inactive native cuts. It does not prove complete native holdings/items,
authenticated cutover flags, full world gameplay, whole lifecycle installation,
shared-journal atomicity, or receipt-required origin discovery. During this
frozen qualification, primary published
`cd52a4a4b017ddc3ac1cf62c7b717e44f75e3c13`, native tree
`7046cb316461c41af6881db298b8789bcc373971`, with version3 catalog origin and
central lifecycle registration. Its native origin values and invariants follow
the [exact primary-owned interface handoff](PLAN5_LIFECYCLE_RECEIPT_DISCOVERY_HANDOFF_2026-10-04.md).
That source is explicitly unqualified: matching independent v3 readers,
required-file discovery, compatibility/fault recovery and newly paired managed
restore proof remain pending. This slice does not qualify that newer tree.
Manifest-anchored loss detection does not discover a receipt already absent
before capture, or authenticate a coherently rewritten/resealed generation.

Financial/alias retention, export/erasure governance and immutable source-name
bindings remain undecided; no redaction or per-record purge policy is invented.
Off-site SSHFS transport, trusted backup-generation provenance, complete workload
budgets, SQL fresh/upgrade/restore on both engines, current combined writer census
and all R1–R8 release gates remain required. SQL is not exercised by this slice;
previous engine evidence does not qualify this merged source.

No shared coordinator/contracts/producers/registry/matrix/activation/migrations
are edited independently. Native wallet-root exclusions and the declined
inactive spell change are preserved. No accounting activation, production data
mutation, deployment, PR merge, experimental-accounting push or audit repair
occurs. The branch alone is published for primary integration.

The ignored local manifest `tmp/plan5/lifecycle-backup-evidence.json` verifies
1248 native files,236 migration files,99 mini runtime assets and23 consumed
fixture/tool inputs. It hashes151 slice artifacts and verifies9243 distinct
previous artifacts remain intact. The earlier failed attempts are preserved;
the private disposable candidate/generation directories are cleaned by the
existing test cleanup after their checks, while exported evidence and tools remain.

The curator notebook update remains unresolved because `AI_CONTEXT.md`, the
required workflow and notebook target are unavailable. This repository report
does not substitute for that update. Plan5 and full release remain open.

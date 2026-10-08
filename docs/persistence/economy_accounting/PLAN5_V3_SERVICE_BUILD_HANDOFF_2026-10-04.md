# Plan 5 refreshed v3 service-build handoff

The separate worktree is
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`, branch
`codex/accounting-plan5`. This follow-up refreshes primary
`a265a505385ef15a86276abd198a618d16685c6e` and preserves the completed SQL
binding slice `1afe82fb615339be1ccf5dcbf51ddda2572d9faa`. Its merged base is
`05b2f7b3e231f5aa8464f162730483b173d36f2e`, native tree
`9e364315847e320d0458a3e750e98aa1f721ec35`, migration tree
`1b0f9a40fef29de409338ba83be015cd3390c9f5` (canonical 0056).
The result/publication SHA and exact consumed inputs are bound in
`tmp/plan5/lifecycle-v3-service-evidence.json`.

Only this report is edited independently. All shared native/contract/producer/
registry/matrix/test-registration files remain unchanged after adopting the
published primary. This is a compiler-defect and remaining-gates handoff;
the managed service journey is still unexecuted.

## Fresh maintained build and actual failure

Primary published the collector row signedness repair and central lifecycle
inventory count corrections. Plan5 therefore attempted the previously blocked
managed v3 backup/restore case on this exact combined source. Earlier passing
v2/older-source service runs do not qualify it.

Using the existing Ubuntu24.04/Python3.12.3/GCC13.3.0 image
`duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
the checkout is read-only, only `bin/` is writable, and Docker networking is
disabled. A fresh explicit object directory and binary path preserve every
earlier build/evidence artifact. No runtime `.env`, database or game service is
used during compilation.

```text
make -C src -j2 PERSISTENCE_BACKEND=flatfile \
  OBJDIR=/workspace/bin/tests/plan5-lifecycle-v3-service/objects-flatfile \
  DMS_BINARY=/workspace/bin/tests/plan5-lifecycle-v3-service/server-flatfile
```

The native collector TU compiles with the maintained strict warning profile,
including `-Werror`; its original signed/unsigned error is closed at compilation.
Its negative/zero/foreign/unrepresentable publication cases remain primary-owned
runtime qualification. The fresh full build emits305 compile commands and then
exits2 at:

```text
flatfile/flatfile_economic_runtime.c:56:42: error:
'current_ownership_epoch' is not a member of 'player_save_execution_guard'
```

No server binary is produced. The managed backup/restore test is not invoked,
no service boots occur and no current-candidate runtime pass is claimed.
`bin/tests/plan5-lifecycle-v3-service/build-flatfile.log` preserves complete
compiler arguments and diagnostics, including the successfully compiled collector.

## Narrow primary header/interface request

The existing API is
`player_save_execution_guard::current_ownership_epoch() noexcept -> uint64_t`
in `src/player/player_save_replay_ownership.h:158`. The failing consumer
`src/flatfile/flatfile_economic_runtime.c` includes only
`player/player_save_execution_guard.h` at line7. That lower-level header holds
the state but does not declare this observer. Other source consumers explicitly
include the ownership header.

The minimal proposed primary fix is to use the existing
`player/player_save_replay_ownership.h` include in this consumer; it already
includes the execution guard. No new function, field, schema, wire format,
duplicate observer or direct access to `detail` is needed. Plan5 has not edited
the shared source or supplied a fake declaration.

The required field/invariant is the actual `detail::ownership_epoch` uint64
observed under its existing mutex. The existing observer returns that value
even after integrity refusal: a poisoned enabled epoch must not become zero
and permit an inactive/unowned bypass. Shutdown must continue to preserve the
runtime projection for a foreign PID/process, unheld coordinator lifecycle
guard, present save-ownership epoch, or initialized/running/append/worker/refused
coordinator state. Only the same-process successfully closed lifecycle can
clear the projection. No persisted authority or accounting activation changes.

A diagnostic-only syntax compile reuses the exact failing TU's maintained flags,
adds `-include player/player_save_replay_ownership.h`, directs dependency output
to the fresh `bin/` directory, and succeeds with exit0. Source bytes are unchanged.
The captured argument array, source SHA and result are in
`runtime-include-probe-second.json`; its output log is empty. This injected-header
probe establishes the header mismatch and proposed resolution, not a passing
candidate build or runtime journey. The first probe's dependency output selected
the read-only source directory and failed; its original log/JSON are preserved
separately. The corrected probe changes only the diagnostic output path.

Required primary checks after the actual include fix: both maintained backend
builds, original save-ownership/integrity and lifecycle shutdown/refusal cases,
then the unchanged Plan5 managed v3 case below on one pinned source. Confirm
nonzero enabled/poisoned epoch preservation, refused/cancelled close preservation,
wrong-process refusal and clean closed-owner projection release. Do not weaken
the observer or shutdown predicates to obtain a build.

```text
DURIS_RUN_BACKUP_INTEGRATION=1 \
  DURIS_PLAN5_LIFECYCLE_BACKUP_SERVER=/workspace/bin/tests/plan5-lifecycle-v3-service/server-flatfile \
  DURIS_PLAN5_LIFECYCLE_BACKUP_ARTIFACTS=/workspace/bin/tests/plan5-lifecycle-v3-service/native \
  PYTHONPATH=/workspace/tests/async \
  python3 -u -m unittest test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration
```

The existing service test needs CAP_SYS_ADMIN and an unconfined seccomp profile
for its private tmpfs/user/network/PID namespaces, with Docker networking disabled.
It checks two actual cold boots, pending native transaction/WAL recovery, immutable
retained receipt preservation, allocator/witness advancement, journal draining,
generation retention and manifest-valid required receipt loss before capture.
It deliberately models inactive native-codec origins and does not execute native
source capture, lifecycle installation or activation. Those gates remain distinct.

## Current central checks

- `python3 tests/async/test_data_lifecycle_manifest.py`:22 PASS,64.077 seconds,
  zero skips. The primary's51 non-database/36 protected-store count correction
  now passes, including the existing protection/reset/lifecycle mutation checks.
- `python3 scripts/generate_economy_writer_coverage.py --check`:PASS,
  `coverage_complete=false`. Reproducible inventory is not writer execution proof.
- `python3 scripts/validate_economy_accounting.py`:exit1,
  `writer source site missing from census`.
- `python3 scripts/validate_economy_accounting.py --release`:exit1,
  `writer has no executable evidence`.

The normal gate retains five exact missing declared triples, all in
`src/economy/coin_physical_publication.c`: `coin.retained_pile_rendering` declares
`(115, coin_bulk_mutation)` and `(124, coin_bulk_mutation)`;
`coin.retained_room_projection` declares `(359, coin_bulk_mutation)`,
`(366, item_publication)` and `(368, item_lifecycle)`. Their JSON and the gate
logs are preserved in the fresh artifact directory. Primary owns correcting
reviewed reachability/site bindings and regenerating the existing matrix;
Plan5 makes no independent registry edit or writer promotion.

All source/migration/runtime inputs, failed build objects/logs, diagnostic probe,
central results and earlier evidence preservation are recorded in the ignored
consolidated manifest. This turn's remaining build blocker is the actual missing
header/API visibility, not the repaired collector signedness or count assertions.
The complete world/holding/UID capture, original atomic/fault/gameplay journeys,
SQL receipt-preimage handoff, trusted backup/retention/erasure/export provenance,
measured workloads and combined R1-R8 release gates remain open.

The primary maintains the shared notebook locally as confirmed by the user;
this report supplies its curator evidence and notebook access is not blocking.
Only the Plan5 branch is published. Accounting remains inactive; wallet-root item
exclusions and the declined inactive spell-path change are preserved. No production
data, service deployment, PR merge or audit auto-correction occurs.

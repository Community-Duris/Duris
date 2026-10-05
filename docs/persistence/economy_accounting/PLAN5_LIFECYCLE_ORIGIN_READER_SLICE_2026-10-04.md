# Plan 5: independent retained lifecycle origin and required-file discovery

Branch `codex/accounting-plan5`; worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Refreshed primary `87b838582387592e3da2dbd0f1b09422aee3de3a` is imported
by a normal history-preserving merge in slice base
`14c8488002132780facaa20d69997305632e521b`, with peer parent
`94e81480b197ebaa9b7cbbabc2c624fabb0198c1`.
Native tree `6e3a31135e47d28e711777f53695f057672f4db6`; migration tree
`1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical0056.
The result SHA and remote readback are in the delivery and ignored local manifest.
At delivery refresh, primary `4c2abb3297742e3c48661616a1b424d53f48b8e2`
imports the previous backup slice. Its native and migration trees match the
frozen inputs above; that integration does not solve the build/qualification
failures recorded below. The active reader run never adopts changing source.

## Owned consumer change

The older independent catalog decoder accepts only v1/v2 and cannot consume
the primary's new v3 state. Present-file scanning cannot independently discover
which original lifecycle receipts must exist. The primary's authenticated
initialization-origin contract now supplies that missing discriminator.

The independent bounded decoder accepts v3's existing160-byte row, validates
the origin enum/reserved bytes, known-origin initialization state, lifecycle
creator/initializer/transition and uniqueness invariants, and retains unknown
origin for v1/v2. Every retained lifecycle-origin initializer requires its
original `lifecycle-<operation>.elr`; original file validation still checks
baseline, common receipt, ordered descriptors and epoch/control links. A known
generic baseline conflicts with a lifecycle receipt, even when all IDs coincide.
Older unknown receipts may remain structurally readable without promotion.

Bounded operator JSON adds `unknown_initialized_origins`,
`baseline_participant_epochs`, `lifecycle_owner_epochs` and
`lifecycle_provenance_complete`. Existing baseline counts and
`baseline_provenance_complete` retain their meanings. Copied-candidate preflight
and post-replay qualification require complete baseline and lifecycle-origin
provenance. The pure audit remains read-only and can report readable unknown
history, while restore refuses that incomplete qualification. No aliases or
source capsules are exported by this aggregate report.

Owned production-tool files are the four existing independent qualifier files:
`scripts/qualify_flatfile_economic_authority.h`,
`scripts/qualify_flatfile_economic_lifecycle.h`,
`scripts/qualify_flatfile_economic_records.h`,
`scripts/qualify_flatfile_restore.cpp`. Owned test changes are
`tests/async/flatfile_restore_lifecycle_receipt_fixture.cpp`,
`tests/async/test_flatfile_restore_lifecycle_receipts.py`,
`tests/async/test_flatfile_restore_baseline_markers.py`,
`tests/async/test_persistence_backup_integration.py`; this report is also owned.
No shared native/server/contracts/coordinator/producer/registry/matrix/activation
or migration files are edited independently.

The native fixture uses the primary's private lifecycle initializer for modeled
lifecycle receipts and the generic initializer for the same-ID counterexample.
It still does not call lifecycle install, capture authenticated native sources,
select an active epoch, or establish the complete shared-journal fault matrix.
It does not patch serialized native origin bytes to make healthy v3 fixtures.

## Exact validation and evidence

Validation uses the existing disposable Ubuntu24.04/Python3.12.3/GCC13.3.0/
OpenSSL3.0.13 image `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Source is read-only; only `bin/` is writable; Docker network is disabled.
The retained actual native v2 fixtures and previous qualifier come from the
completed reader slice's `bin/tests/plan5-lifecycle-reader/final2`; actual v1
native inputs come from `bin/tests/plan5-baseline-marker/native-v1-7f64`.
The v2 fixtures were emitted at reader result
`9306818b5e3102aa692a35a5ec8a1b1a84d87efa`, native tree
`84fd8dc7329646d6a32760c1be53e0ddcea1a533`. The actual v1 compile consumes
preserved source at `7f64e18902ce167a8f01636c8c46a6fec1767edd`, native tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`.
Compatibility fixtures, older tools and prior evidence remain preserved.

```text
make -C src -j2 PERSISTENCE_BACKEND=flatfile \
  DMS_BINARY=/workspace/bin/server/plan5-lifecycle-origin-flat-dms_new
PYTHONPYCACHEPREFIX=/workspace/bin/tests/plan5-lifecycle-origin/python-cache \
python3 -u tests/async/test_flatfile_restore_lifecycle_receipts.py \
  --artifacts /workspace/bin/tests/plan5-lifecycle-origin/second \
  --previous-qualifier /workspace/bin/tests/plan5-lifecycle-reader/final2/qualify \
  --legacy-artifacts /workspace/bin/tests/plan5-lifecycle-reader/final2 \
  --state-parent /dev/shm
python3 -u tests/async/test_flatfile_restore_baseline_markers.py \
  --native-source /workspace \
  --legacy-native-source /workspace/bin/tests/plan5-baseline-marker/native-v1-7f64 \
  --artifacts /workspace/bin/tests/plan5-lifecycle-origin/marker-regression
python3 tests/async/test_data_lifecycle_manifest.py
python3 tests/async/test_flatfile_backup_manifest.py
python3 scripts/validate_economy_accounting.py
python3 scripts/validate_economy_accounting.py --release
python3 -m py_compile tests/async/test_flatfile_restore_lifecycle_receipts.py \
  tests/async/test_flatfile_restore_baseline_markers.py \
  tests/async/test_persistence_backup_integration.py
./scripts/format.sh --check
git diff --check
```

Evidence is under `bin/tests/plan5-lifecycle-origin/`. The lifecycle matrix passes
136 cases:122 structural/refusal cases and14 readable accepted cases, including
seven readable unknown-origin cases that copied-candidate qualification refuses.
The marker regression passes71 cases:56 structural refusals, nine readable
unqualified cases and six complete-provenance cases. Both suites have zero skips.
Each lifecycle case exercises sanitizer pure audit, bounded operator audit,
copied-state preflight and post-replay qualification, while verifying bytes,
mode, links, inode, size and mtime remain unchanged at the appropriate gates.
The independent reader includes no native mutation/recovery codec.

Both independently built full qualifiers have SHA256
`0bd6ffd43d69a9c9a37cecdbe1c25515b3084ca1c47a9d36eb0aacbf491f53a8`.
The lifecycle ASan/UBSan pure-audit binary has SHA256
`5f66addc6b2bfe72b55b0948f150f6ee025e0d278b1f71300987e7fd2bbbe991`;
the actual native lifecycle-codec fixture has SHA256
`504cf7b155fd678b22602c7b2698ab6f9d4df388df1d78e456bfc1ac5ef4dc53`.
The previous source-paired v1/v2 qualifier refuses healthy native v3 at all three
production-tool gates, establishing the compatibility gap without writing state.
Actual retained v2 receipts remain structurally readable, but neither their
presence nor absence promotes initialization origin. Native generic same-ID
initialization remains generic and complete without a lifecycle receipt.

The first lifecycle run reached the new discovery/legacy checks, then a test
loop accidentally shadowed the fixture path variable. The corrected complete
run is `second`; the incomplete `first` log/tools/fixtures remain preserved.
No missing API or compiler failure is counted as a semantic RED. The first Python compile
attempt used a read-only default cache path and refused; the explicit ignored
`PYTHONPYCACHEPREFIX` compile succeeds. Changed-line and whole touched-file
formatting pass using WSL clang-format14 and explicit managed-worktree Git paths.
Four existing flat-file backup manifest tests pass in0.429 seconds, zero skips.
The ignored consolidated manifest `tmp/plan5/lifecycle-origin-evidence.json`
verifies1253 committed native files,236 migration files,1236 exact legacy v1
source files, both suites' consumed inputs and binary hashes,6047 slice artifacts
and preservation of9394 previous artifacts. The managed v3 service journey and
full release flags remain false; compiler, inventory and validator failures are
retained separately from the passing independent matrices.

## Primary-owned build repair handoff

The required maintained build fails on this exact native tree with:

```text
economy/collector_service.c:538:34: error: comparison of integer expressions
of different signedness: 'int' and 'unsigned int' [-Werror=sign-compare]
if (selected->db_item_id != (flat ? 0 : result.materialized_item_id))
```

Exact fields: `obj_data::db_item_id` is signed `int` in
`src/core/structs.h:523`; `collector_command_result::materialized_item_id`
is `uint32_t` in `src/economy/collector_command.h:83`. Consumer is the native
collector purchase publication verifier `purchase_effect` in
`src/economy/collector_service.c`. No schema or interface expansion is needed.

Preserve the invariants: flat publication requires native row0; SQL publication
requires the exact positive native row and rejects negative native IDs or
unrepresentable unsigned results. Keep existing materializer/body/UID/owner
proof, retry behavior and inactive gameplay semantics. A backend-specific
comparison with a checked nonnegative signed value can avoid unsafe narrowing;
do not suppress the required warning or cast an unbounded unsigned result to int.
Primary owns this source fix and collector native tests. Required proof includes
the maintained flat build, corresponding SQL build and native publication cases
for flat0/nonzero, matching/foreign SQL rows, negative IDs and values above INT_MAX.

Build failure is preserved in `bin/tests/plan5-lifecycle-origin/build-flat.log`.
The primary's central registration also leaves two existing inventory assertions
stale in `tests/async/test_data_lifecycle_manifest.py`: line112 expects50 stores
but reports51; line261 expects35 protected recovery stores but finds36. The
22-test run has20 passes and those two failures, preserved in
`lifecycle-manifest-unit.log`. Primary should update the registered family/count
contract and execute the existing per-store recovery protection mutations,
including `file:economic-lifecycle-receipt`, without weakening retention guards.
No independent central inventory/test edit is made in this slice.

Normal accounting contract validation also refuses `writer source site missing
from census`; the five stale `writers[*].sites` triples are in
`src/economy/coin_physical_publication.c`: route `coin.retained_pile_rendering`
declares `(115, coin_bulk_mutation)` and `(124, coin_bulk_mutation)`;
`coin.retained_room_projection` declares `(359, coin_bulk_mutation)`,
`(366, item_publication)` and `(368, item_lifecycle)`. Primary owns correcting
those reviewed reachability/site bindings and regenerating the existing matrix.
The invariant is exact membership of each declared `(path,line,family)` in the
current census, not invented or promoted runtime evidence. Release mode refuses
`writer has no executable evidence`; the matrix has `coverage_complete=false`
and retains historical anchor `5f542e9cf769dfa7ef89e28ce4845d7495d4e1b3`.
Both validator failures are preserved in `contract-gate.log`/`release-gate.log`.

The new managed test also captures a checksummed generation after its required
receipt is already absent, then requires independent refusal before service boot
and unchanged source/generation. Its source is implemented, but the managed
service journey on this combined tree awaits a buildable primary candidate.
The earlier two-boot source-specific slice does not qualify this newer source.

## Remaining gates

Native initializer/receipt/book/selection atomicity, allocation/fault recovery,
full retained/current-source startup and gameplay belong to primary qualification.
Independent reader cuts do not establish authentic holdings/items/cutover, trusted
backup provenance or complete lifecycle installation. Coherent resealed origin
rewrites retain the external authoritative-source/generation authentication gate.
Unknown provenance cannot earn complete lifecycle preservation qualification.

SQL fresh/upgrade/restore on both engines, financial/alias retention and export/
erasure governance, trusted/off-site transport, full workload budgets, combined
writer census and R1–R8 release remain required. SQL and accounting activation
are not exercised by this slice. Wallet-root item exclusions and the declined
inactive spell path are preserved; no production data, deployment, PR merge,
audit correction or independent experimental-accounting push occurs.

The curator notebook update remains unresolved: `AI_CONTEXT.md`, the required
workflow and notebook target are unavailable. This repository report does not
substitute for that update. Plan5 and release remain open.

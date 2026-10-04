# Plan 5: authority-bound baseline initialization readers

Branch `codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Base `35862bf6d7f036ef1003285be0e4c19f41c152e8`, the history-preserving merge
of Plan5 `7f64e18902ce167a8f01636c8c46a6fec1767edd` and published primary
`786285cb3e8e1f45f31d915ca029943dbeccdc00`.
Result commit and committed input hashes are recorded after commit in
`tmp/plan5/baseline-marker-evidence.json` and the chat handoff.

Current fixture and maintained build source is the published native tree
`84fd8dc7329646d6a32760c1be53e0ddcea1a533`. It already contains the marker
implementation `0aa0bceeba95c39e1ab90810057f5284463196dc` and staged lifecycle
composition `2b4591c21`; an earlier source note incorrectly described the marker
implementation as unpublished. No native or migration files are changed by
this slice. Migration tree remains `ee51e2c36250ffda10c01170555f383229cd731c`,
including incoming canonical `0056_spell_ward_durability`.

## Established defects and the complete reader change

The independent reader accepted only catalog envelope v1. On the exact published
native tree, the original full verifier refused an intact native v2 empty book
in both preflight and post-replay modes. Its binary SHA256 is
`ae046c5479939c7fb756714d183434dbef5deb0d8e747c818da6a25cf1447459`.
Evidence is `bin/tests/plan5-baseline-marker/red-v2-current.json` and its log.

The earlier v1 native tree `d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`
at Plan5 `7f64e18902ce167a8f01636c8c46a6fec1767edd` demonstrates the original
discovery gap. The original sanitized reader and both full verifier modes
admitted complete deletion of an initialized empty book, substitution of its
head initialization ID, and substitution of its opening context. The intact
book also passed. The successful reproduction recreates every private file as
0600 and changes no retained bytes during reads. Its four-case record is
`bin/tests/plan5-baseline-marker/red-v1-corrected/evidence.json`.

One independent bounded catalog decoder now serves authority validation and
baseline discovery. Only `epochs.eae` accepts envelope v2. It validates all
160-byte descriptors, including state, seven reserved bytes, original
initialization operation ID and exact canonical opening key. Unknown/never
states require zero proof; initialized requires a nonzero ID and canonical
kind9 key with matching lineage and nonzero authority. The complete encoded
catalog must match the digest in `authority.eal`. V1 descriptors remain
`legacy_unknown` and are never inferred to be initialized or never initialized.

Every initialized marker seeds book validation, including earlier retained
epochs and inactive selections. Entire empty-book loss therefore reaches the
same head/index refusal as partial loss. Every surviving book and successful
baseline receipt must belong to a retained marker; a known-never marker cannot
have a book. An initialized head's opening key must exactly match its marker,
including context. Revision-zero terminal operation must be the original
initialization ID. Populated heads keep their later batch operation ID and
continue to match the complete ordered witness/reservation/receipt history.

## Read-only operator and qualification behavior

Build with `python3 scripts/build_restore_qualifier.py`, then run:

```text
bin/tools/qualify_flatfile_restore --economic-evidence-audit /absolute/state
```

This branch validates the existing root and calls only independent readers,
before persistence configuration or native journal recovery. It requires no
`ISOLATED_RESTORE` marker and does not create directories, repair data, activate
an epoch, initialize a book, or write an adjustment. It reports aggregate
`legacy_unknown_epochs`, `never_initialized_epochs`, `initialized_epochs`, and
`baseline_provenance_complete`. A structurally intact legacy store is readable
with the last field false. Damaged structures return the fixed refusal and no
report. No aliases, player IDs or payloads are printed.

Full copied-candidate preflight and post-replay qualification refuse unknown
initialization provenance. They cannot certify complete book preservation
from missing files in v1/unknown history. The audit mode preserves structural
inspection of that history. Native gameplay/producer behavior is unchanged.
Neither the boolean nor a passing verifier authenticates a coherently rewritten
catalog/control/book or establishes a complete release. External source and
backup-generation attestation remain required.

## Owned files and interfaces

Only these seven files belong to the commit:

- `scripts/qualify_flatfile_economic_authority.h`
- `scripts/qualify_flatfile_economic_baseline.h`
- `scripts/qualify_flatfile_economic_records.h`
- `scripts/qualify_flatfile_restore.cpp`
- `tests/async/test_flatfile_restore_economic_authority.py`
- `tests/async/test_flatfile_restore_baseline_markers.py`
- this report.

The native fixture, build helper, mutation codecs, shared journal/coordinator,
SQL contracts, migrations, producer integration, writer registry/matrix and
activation owner are untouched. The existing fixture builder can explicitly
consume a complete separate native source tree; it uses that tree's headers
and source paths without mixing checkout headers or cached checkout objects.
No shared format extension is requested: this consumes the exact fields and
invariants in [the marker-v2 handoff](BASELINE_INITIALIZATION_MARKER_INTERFACE_V2.md).
Consumers are the authority cross-link reader, baseline namespace reader,
read-only operator mode and both copied-candidate qualification modes.

## Exact validation and evidence

All native commands used network-disabled disposable Docker containers with
read-only `/workspace` source and only its `bin/` overlay writable. No database
or game service was contacted. Image `duris-plan5-origin-sql-tools:local` has ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`;
Ubuntu24.04, Python3.12.3 and GCC13.3.0. Native fixture and independent reader
use C++20, strict warnings and ASan/UBSan with leak detection and halt-on-error.
The full native verifier is built client-free with `__NO_MYSQL__` and links
OpenSSL/zlib/pthreads. Current source has 1,248 raw tracked native files;
legacy source has 1,236. Each source's raw hashes are retained and verified
unchanged before/after the marker suite. Eight reader/fixture/build/test input
hashes are frozen in `tmp/plan5/baseline-marker-frozen-inputs.json`.

```text
python3 -u tests/async/test_flatfile_restore_baseline_markers.py \
  --native-source /workspace \
  --legacy-native-source /workspace/bin/tests/plan5-baseline-marker/native-v1-7f64 \
  --artifacts /workspace/bin/tests/plan5-baseline-marker/green
```

PASS, 72 cases, 57 structural refusals, nine readable/unqualified histories,
six cases passing the narrow baseline provenance check, zero skips. The six
include two explicitly synthetic format/boundary checks; they do not establish
authentic provenance. Native v2 fixtures cover an empty catalog, known-never
epochs, an initialized empty earlier epoch and populated books in two inactive
retained epochs. Native v1 empty/populated fixtures remain structurally readable
but unqualified. Each case runs a separate sanitized independent reader, the
read-only operator, native preflight and native post-replay verifier. State
bytes/modes/links/inodes/mtime remain unchanged during both independent/operator
reads; economic evidence remains unchanged during both full verifier modes.

Refusals include entire empty/nonempty book loss, all17 individual empty-book
file losses, head ID/key substitution, stale digest, malformed state/reserved
bytes/key/zero proof, known-never books/receipts, capacity4097, malformed
catalog order/chain/duplicates and non-catalog v2. The maximum4096 descriptor
catalog is655432 bytes. Downgraded/unknown histories never gain qualification.
The full record and frozen native stores are under
`bin/tests/plan5-baseline-marker/green/`; stdout is `green.log` in its parent.
An additional operator smoke check passes without an `ISOLATED_RESTORE`
marker while a deliberately corrupt pending domain journal remains byte-for-byte
unchanged. Missing and symlink roots refuse, without creating or changing state.
The three checks have zero skips; `operator-smoke.json`/`.log` retain the exact
toolchain and outcomes, including OpenSSL3.0.13.

```text
python3 -u tests/async/test_flatfile_restore_economic_authority.py
```

PASS, all18 original valid stores and317 original corruption refusals, zero
skips. Both catalog strides retain the original predecessor/duplicate cases.
There are1,005 native operator/preflight/post-replay invocations and335
sanitized independent checks. This preserves the original malformed authority,
retained operation, baseline/witness/reservation, maximum holding/item forest,
65536-entry reservation shard and source-claim cases. Economic evidence is
unchanged. Record: `bin/tests/plan5-baseline-marker/regressions.log`.

Both suites built the same full verifier, SHA256
`75911d08edc567626360b0e9f22784e789789d81930517fdbda766a092a30071`, and
native current-source fixture, SHA256
`1a91ad04f9952857bb97e29d885c74795a673f6455d7b7bd31ac4c47a239d90f`.
The marker suite sanitized reader is
`bff34452bc7af36bedccc7f0e5bd89ad3d6a0e23de53343217703c52a680eab3`;
the retained original-matrix sanitized reader is
`17b8aefc4c99ec12559d11e5ab12a53aae51f9fca71459f2d2db25844e81ca21`.
Legacy native fixture is
`82b4f71d64a3262bf958292c7e6dc1d9b9134b7f23bd887c5b652dc2954eac82`.

```text
make -C src -j2 DMS_BINARY=/workspace/bin/server/plan5-marker-published-dms_new
./scripts/format.sh --check
python -m py_compile tests/async/test_flatfile_restore_baseline_markers.py tests/async/test_flatfile_restore_economic_authority.py
git diff --check
```

All PASS. The maintained build is MariaDB/development with the repository's
strict warning profile, with no server boot or SQL execution. Server SHA256
`c8ef654edd4f62a463ec53410fe39fc11dd3135a78a84f10ce110ffc02929351`.
Build and formatting logs are `build-published.log` and `format-check.log`
under `bin/tests/plan5-baseline-marker/`. Formatting used WSL
clang-format14 and explicit Linux `GIT_DIR`/`GIT_WORK_TREE` for the Windows
managed-worktree link. The initial automatic Git lookup failed; a full format
of only the four owned touched C++ files and the final repository check passed.
All692 previous namespace/prior-slice artifacts still match their hashes.

Preserved setup failures: `red.log` assumed v1 in a native v2 fixture;
`red-v1.log` and `red-v1-combined.log` recreated private metadata with public
permissions, so the old reader properly refused before the defect cut. The
corrected reproduction uses077 umask/0600 assertions and the pinned previously
built binaries. Failed artifacts/logs are retained. No failure is counted as a
successful defect reproduction or waived case.

## Shared handoff and remaining gates

`python3 scripts/validate_economy_accounting.py` exits1 at
`writer source site missing from census`. There are five absent site tuples:
`coin.retained_pile_rendering` at `src/economy/coin_physical_publication.c`
lines115/124 (`coin_bulk_mutation`), and `coin.retained_room_projection` at
lines359 (`coin_bulk_mutation`),366 (`item_publication`),368 (`item_lifecycle`).

`python3 scripts/validate_economy_accounting.py --release` exits1 earlier at
`generated writer route inventory drift`: ten inventory routes are absent
from the matrix, with no extras. They are the two coin routes above,
`lifecycle.flatfile_activation_install`, `recovery.inert_literal_cleanup`,
`recovery.inert_literal_eligibility`, `recovery.inert_literal_staging`,
`recovery.ordinary_drop_graph_observation`,
`recovery.ordinary_drop_graph_reconstruction`,
`recovery.runtime_owner_revision_observation` and
`recovery.sql_ordinary_drop_receipt_observation`.
The primary owns census/matrix reconciliation and corresponding source-bound
native route proof. No registry/matrix edits are included here. Later release
checks were not reached; fixing these inventories alone cannot qualify them.
Records: `release-gate.log`, `release-contract.log`, `census-missing-sites.json`.

Native marker atomicity/retry/journal/OOM cases, complete lifecycle installer
qualification, independent immutable lifecycle receipt discovery/cross-checks,
external source attestation, full SQL/flatfile gameplay and cold restart/restore/
retention journeys, budgets and applicable R1–R8 gates remain open. No MySQL8
or MariaDB disposable-database execution or maintained flatfile server build
was repeated for this flatfile reader slice. Earlier source-specific database
and0055 results do not qualify this combined candidate. No skip waives a gate.

The notebook curator workflow remains unavailable: `AI_CONTEXT.md` is absent,
and the previous request has not supplied a curator target or workflow. This
repository handoff is not claimed as a notebook update. The Plan5 goal stays
active. Accounting remains inactive; wallet-root exclusions and the declined
inactive spell-path change are preserved. No activation, production mutation,
audit correction, deployment, merge or independent experimental-branch push
is performed.

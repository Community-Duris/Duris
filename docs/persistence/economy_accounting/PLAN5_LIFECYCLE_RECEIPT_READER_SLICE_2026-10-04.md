# Plan 5: independent validation of retained lifecycle receipts

Branch `codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Slice base `42594db011f31fdd1a6421c56994699b4ee848de`; result is the commit
containing this report, with full SHA and remote readback in the delivery and
local `tmp/plan5/lifecycle-reader-evidence.json`.
The preceding interface request is independently committed in that base.

The source consumed by this slice has native tree
`84fd8dc7329646d6a32760c1be53e0ddcea1a533` and migration tree
`ee51e2c36250ffda10c01170555f383229cd731c`, including canonical0056.
Its primary ancestor is `b21b0c28dec5a49ad1ebb98d965cdff5bc3f2e59`, merged
without a content change in `4e786c49ba239923de66467b585876208ed4c38e`.
The previously published native marker/receipt implementation is already in
this source; this slice does not qualify an unpublished prerequisite.

A subsequent read-only refresh found primary
`46e37a1fc47ce5e8d5de82f4455fd1d048ae033f`, native tree
`939f53adc32e290e8b285f1ab606dd7df7dd02e5`, with the ordinary-drop census fix
and updated review checkpoint. The running source remained frozen. This report
does **not** qualify that newer combined candidate or any later integration.

## Established defect and complete fix within the present-file scope

The independent record reader ignored every `lifecycle-*.elr` file. The preserved
source-paired old qualifier SHA256
`75911d08edc567626360b0e9f22784e789789d81930517fdbda766a092a30071` admitted
an invalid lifecycle filename, corrupt envelope and contradictory assertion
flags at operator audit, copied-state preflight and post-replay qualification:
nine executed RED observations, retained evidence unchanged.

The new independent reader validates every present lifecycle-prefixed file:
canonical original-ID filename, private regular single-link file, bounded v1
envelope, exact body size/hash/tail, immutable catalog epoch/control joins,
original mapping identities and ordered wallet/bank source descriptors, derived
baseline operation, native source fingerprints and complete coverage digest,
no-item EAB witness and canonical baseline command. It requires an exact matching
successful common root with identical command/plan capsules and durable revision,
zero failure stage and empty result, then uses the existing independent baseline
reader to verify all retained book/index/witness/effect relationships.

Historical aliases and mapping revision/operation metadata remain original
receipt authority. Only stable account identity is joined to current retained
mappings; later bank renames and wallet retirement do not reconstruct or rewrite
the original descriptors. Lookups use an eight-bucket cache. Receipt count is
bounded by4096, mappings by3071, aliases by50 bytes and encoded receipt by6377936
bytes. The operator adds only aggregate `lifecycle_receipts`; no private aliases
are emitted. The independent header includes no native codec/storage/mutation
API and has no recovery, installation or correction path.

Owned files only:

- `scripts/qualify_flatfile_economic_lifecycle.h` (new independent reader).
- `scripts/qualify_flatfile_economic_authority.h` (retain decoded immutable
  epoch fields and bounded historical account lookup).
- `scripts/qualify_flatfile_economic_records.h` (receipt/common-root linkage).
- `scripts/qualify_flatfile_restore.cpp` (aggregate operator count).
- `tests/async/flatfile_restore_lifecycle_receipt_fixture.cpp` (new native codec
  and common-baseline primitive fixture).
- `tests/async/test_flatfile_restore_lifecycle_receipts.py` (new executable cuts).
- `tests/async/test_flatfile_restore_baseline_markers.py` (new header input pin
  and aggregate output expectation).
- This report.

No native server, shared contract/coordinator, producer, registry/matrix,
activation owner, migration or central lifecycle manifest is edited.

## Exact executable evidence

Docker image `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`:
Ubuntu24.04, Python3.12.3, GCC13.3.0, OpenSSL3.0.13. Containers use
`--network none`, source mounted read-only and only `bin/` writable. No `.env`,
production service, database or game account is read or contacted.

```text
python3 -u tests/async/test_flatfile_restore_lifecycle_receipts.py \
  --artifacts bin/tests/plan5-lifecycle-reader/final2 \
  --state-parent /dev/shm \
  --previous-qualifier bin/tests/plan5-baseline-marker/green/qualify
python3 -u tests/async/test_flatfile_restore_economic_authority.py
python3 -u tests/async/test_flatfile_restore_baseline_markers.py \
  --native-source /workspace \
  --legacy-native-source /workspace/bin/tests/plan5-baseline-marker/native-v1-7f64 \
  --artifacts bin/tests/plan5-lifecycle-reader/marker-regression
make -C src -j2 DMS_BINARY=/workspace/bin/server/plan5-lifecycle-published-dms_new
./scripts/format.sh --check
python3 -m py_compile tests/async/test_flatfile_restore_lifecycle_receipts.py \
  tests/async/test_flatfile_restore_baseline_markers.py
git diff --check
```

The final new suite passes109 cases:101 refusals, six intact native-codec fixtures
(mixed/empty/renamed/retired/maximum/old-retained), and two accepted discovery-boundary
demonstrations. Every case executes the standalone ASan/UBSan reader, operator
audit, copied preflight and post-replay gates:436 checks, zero skips. Bytes,
mode, link count, inode, size and modification time are preserved. Pure audits
preserve the whole disposable state; restore gates preserve retained evidence.
Native source1248 files and all owned consumed inputs are frozen and rechecked.
The retained fixture appends a newer inactive epoch after the original receipt
and supplies explicit matching requested coverage. It proves that those joins
preserve the old receipt without selection, recapture or mutation by the reader.

The original authority matrix passes18 valid stores and317 corruption refusals:
335 sanitized reader checks and1005 operator/pre/post gates. The marker suite
passes72 cases:57 refusals, nine readable/unqualified cases and six narrow
baseline provenance accepts, zero skips. Actual legacy native input is commit
`7f64e18902ce167a8f01636c8c46a6fec1767edd`, tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b` (1236 source files). Synthetic
coherent/capacity marker cases retain their original limitations.

The fixture calls the **actual** published native lifecycle receipt encoder and
decoder, generic private authority/baseline participants, common transaction
commit and actual baseline lookup. It reads the stored durable revision and
requires the native codec roundtrip to preserve bytes. It never calls lifecycle
install, captures live native holdings, selects an epoch or boots a server.
Wallet/bank source objects and external frozen/virgin flags are modeled. Its
actual native control reader asserts active epoch zero throughout.

The3,071-bank/50-byte-name fixture is actual native codec/common-store output;
it establishes format capacity only. The first disk-backed run reached intact
mixed/empty/renamed/retired cases and then timed out after180 seconds constructing
that fixture. `first.log` and its binaries remain preserved as a failed run.
The completed run uses private disposable `/dev/shm` state with a600-second
construction bound; this is **not** crash-durability evidence or a relaxed
release-host budget. The original RED is reexecuted in the completed run.
The earlier108-case RAM run remains preserved under `green/`. An intermediate
run `final/` was stopped (exit137) after source inspection found an incorrect
member name in the newly added historical fixture. The corrected member is
`predecessor`; only the completed `final2/` run qualifies the final fixture.

Standalone sanitized audit SHA256
`762d1e6e525110ade3f78f27fb3209c833048dd0763f8d80a83c07ce2db7fd35`;
native primitive fixture
`5aaf341cf1ccdb4553fa603fb8b0b77b66c4d274e8d4f1b0f6cfbde84aaf4984`;
new full qualifier
`e897c15ba07bc42191ff79f824ab23b1f7be56b483958a6727904305fe69890d`.
The strict maintained MariaDB/development build passes and matches server SHA256
`c8ef654edd4f62a463ec53410fe39fc11dd3135a78a84f10ce110ffc02929351`.
It is an incremental maintained build/link, with no runtime SQL or gameplay.
Formatting uses WSL clang-format14 with explicit managed-worktree Git paths.
An initial WSL attempt could not create its Docker-owned log; the unchanged
check rerun captured through PowerShell passes. Direct formatting of the owned
C++ files also includes the new untracked header and fixture.

Local evidence root: `bin/tests/plan5-lifecycle-reader/`. `final2/evidence.json`
contains the source fingerprints, case outcomes, binary hashes and nine RED
observations; `final2/native-*` preserves actual native output. Other logs are
`final2.log`, `green.log`, `first.log`, `final.log`, `authority-regression.log`,
`marker-regression.log`, `build.log`, `format.log` and `format-new-files.log`.
These generated artifacts remain outside Git.
The marker regression preserves its separate native-v1/v2 input sets and JSON.
The matrix's temporary sanitizer binary is removed by its existing harness;
its exact hash remains in stdout. No claim of preserving that binary is made.
The consolidated manifest verifies all1248 native and236 migration blobs against
the frozen Git base, and preserves all2450 previously recorded artifacts
(marker1758 plus earlier692). Artifact hashes, final source pins, result SHA and
remote readback remain in the ignored local evidence files, outside Git.

## Remaining gates and shared requests

Required-file discovery is **unqualified**. The executed generic participant
counterexample has equal epoch creation/initialization/preparation IDs, a valid
book/common root and no `.elr`. Removing the entire lifecycle file from the
mixed fixture is still accepted when the authority-bound required-origin
discriminator is absent. Both demonstrations are labeled limitations, not
qualification passes. `baseline_provenance_complete` describes the existing
baseline marker proof only; receipt count supplies no lifecycle completion flag.

The narrow [primary interface request](PLAN5_LIFECYCLE_RECEIPT_DISCOVERY_HANDOFF_2026-10-04.md)
specifies authoritative `initialization_origin`, values/invariants, atomic
ownership, immutable retention, exact consumers and native/independent tests,
or a proved existing disjoint predicate. It also requests primary-owned central
registration of `file:economic-lifecycle-receipt`, its native producer/locator
and private fields. No shared interface is independently implemented here.

Receipt hashes and coherent source bindings do not authenticate externally
asserted cutover fields. Full native holding/item census, wallet-root exclusions,
whole staged installer/fault recovery, historical required-file preservation,
trusted backup generations and retention/export/erasure policy still need their
native/primary qualification. Alias redaction cannot rewrite source fingerprints.
There are no ad hoc retention periods or automatic corrections.

SQL runtime/disposable-database checks are not applicable to this flat-file
receipt-reader change and were not rerun. Earlier both-engine/0055 or0056 slices
do not qualify this source or the newer combined candidate. Complete SQL and
flat-file gameplay/cold-boot/backup/restore journeys, default/release census and
route gates, original R1–R8 budgets and combined release acceptance remain open.
The declined inactive spell change and healthy inactive behavior are preserved.

The notebook curator workflow/target remains unavailable: `AI_CONTEXT.md` is
absent, the prior clarification is unanswered and bounded discovery supplied
no callable curator or matching notebook. This report is a reviewable handoff,
not a notebook update. No notebook completion is claimed.

No accounting activation, production data mutation, deployment, PR merge,
experimental-accounting push or audit repair occurs. The completed slice is
published only to the remotely visible `codex/accounting-plan5` branch for
primary integration. Plan5 and release remain active/unqualified.

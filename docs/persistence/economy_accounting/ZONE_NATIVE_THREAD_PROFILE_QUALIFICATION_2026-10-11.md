# Zone native thread profile qualification - 2026-10-11

This packet independently qualifies the permanent native-birth/Zone profile on
published primary `7ad95475de1e8dd87c7d89fbfba4bc777bb0a49b`, tree
`e08adc3353c1d586afc9a633e30bbd37cde4a441`, exact parent
`221f9e5ac159a0a1498222b348d2cdb05f9677da`. The original registered entry
passes once. Coordinator objects supply compile-only evidence; the actual
runtime assertions cover two public Zone getters. Full accounting remains open.

## Source review and unchanged acceptance

The selected freeze contains all 3,997 regular Git inputs and ten changed paths.
Production source changes only `src/world/zone_reset_item_owner.c`. Complete
inverse correspondence verifies only the documented comment, one three-to-two
native-handle term and the two profile guards; every other provider byte remains.
Both getters now require the actual native thread macro and glibc >= 2.27 instead
of refusing that macro. The direct pthread path declares two native-handle
carriers; its typed thread contribution is 42 bytes on this LP64 profile.
Source review establishes correspondence, without a native parent counterfactual.

The Coordinator and Utility source, Makefile, entry adapter, matrix/checker,
TestResult runner, integration manifest, original Item helper and Utility
profile test remain byte-identical to the parent. All 931 writer policies and
2,911 census entries are identical. Four declared pin values change or are added,
500 prior values remain, and both metadata pin maps agree on all 504 paths.
The new Zone provider, fixture, runner and regression-manifest pins are exact.
All prior regression entries remain unchanged; the sole new registration is
script mode, minimum 1, CPU2/8 GiB recommendation and original 5,400-second deadline.

The operator selects the exact TestSpec from the complete original inventory
and invokes unchanged `tests/run_regression_tests.py::run_test` once. Actual
entry observer/completion checks return real script and main cases. The API
removes its original temporary raw case JSON; the real TestResult, returned
cases and complete output are retained. Full scheduler/resource admission is
not invoked or qualified. The separate single-entry lease grants CPU1/2 GiB;
the original runner retains serial make -j1, genuine production flags, all
assertions and original compile/run ceilings. No acceptance condition changes.

## Actual terminal result and component boundary

Original API exit 0 / status passed in 176.534540 seconds, zero skips; actual
prefixed script and main cases both passed. Four ordinary whole Coordinator
Make objects compile across SQL-header/flat and supported/assertions profiles.
Four separately compiled whole included Zone providers link with section GC
and execute the original public-getter assertions. Eight whole objects total.

| Backend | Whole Coordinator production objects | Supported Zone getters | Genuine assertions-profile Zone getters |
| --- | --- | --- | --- |
| SQL-header (`mariadb`) | supported and assertions compile | both exact values accepted | both refused, output unchanged |
| Flatfile | supported and assertions compile | both exact values accepted | both refused, output unchanged |

Each of four Zone runs includes two null refusals and two strong-output checks.
Actual whole-TU macro probes require `_GLIBCXX_NATIVE_THREAD_ID pthread_self()`
and glibc >= 2.27; no native macro is overridden to manufacture support.
GNU13.3.0/Python3.12.3 run the original installed recipe. The expected fixture
uses the original graph constants plus its typed thread expression. This is
arithmetic/profile conformance, not independent declaration completeness or
transitive caller/library/lifetime coverage. No Coordinator private getter or
birth caller is executed. Zone observer/game/database functions are discarded
at the sectioned link and never called; no full executable link is established.

Original default source contracts exit 0 in 19.770979 seconds. Host
transport exits 0 with no collector/transport error. No previous Item, held,
Utility, full build/matrix, generator, release audit, database or game rerun.
Primary's reported private native artifacts and finite formatted Source reviews
remain published reports only; those inaccessible raw packets are not treated
as independently reviewed proof here.

All 4,030 sealed members / 147,646,108 bytes are
independently rehashed, including all 3,997 unchanged frozen inputs. Original
TemporaryDirectory cleanup removes compiled objects and ELFs after the pass;
there is no retained binary-hash proof claim. Receipt hashes below bind the
actual original result, complete output, source freeze, seal and resource closure.

| Receipt | SHA256 |
| --- | --- |
| `source-inventory.json` | `064094cd7ead16365ddde4697abb89504fcc14616684546ff7cd62a34af06d4e` |
| `source.tar` | `0e7ab620ae061f49409968d295a6e5873438e78651d542b6f4282a2d37e73935` |
| `changed-file-freeze.json` | `35350c8f61333335e4a4433ecd6a4beaa0bdf8ab0d674ae81cf6d9e8fd135c4d` |
| `source-review.json` | `fd29d5850be3581fad73e848e187f736db63fc2a28c42c4d8cd2147ae457ad9d` |
| `preparation-receipt.json` | `40bf02cb499aef3487d562d58642893ee46086004227952e6f2a4132310a9806` |
| `resource-grant.json` | `1cd1e73112a0146b33f4c97bae35db601728ec3b7fbd874fa51d5720ed70bb31` |
| `native/registered-zone-profile.result.json` | `c5a3efa96e978d56e959f2b3270fcf13733313361552ba89bb49adc6c5746673` |
| `native/registered-zone-profile.output.txt` | `c9fa66b5381af7432804546a510d3e9d68d2029fd0847c3c1b8773c8b3298062` |
| `native/native-terminal.json` | `ced433f670de3e72e4f9e647eee9ce761f6a7a243fb003fc5d496594ef2a7a1f` |
| `native/source-contracts.result.json` | `383e23fe932151cabe24829d68634750be7dc2f7244954411d48c10a4dae250e` |
| `evidence-seal.json` | `a79c8f085b3c013fed84e67e219da673ed70471b81093d55f8002b9d2d6b2e5f` |
| `resource-cleanup.json` | `863b3b3efd911e5b852b5a9fabce4131ec5c6ba5d396eecb65740cc01c663c2d` |
| `resource-release.json` | `9e08b320198c00f796253df3812a50a633f1b416fdf3a04bb8e6a4f3aa6ec811` |
| `delivery/terminal-qualification-review.json` | `4c471958315f72998e79fb98f9c1ac0a11b07174e31548d14bb9274da3dcd7b5` |

## Current-base metadata adoption handoff

Raw Git hashes match 503/504 pins. The unchanged adapter's declaration is its
known CRLF encoding alias: declared
`bdfbc6eb466ddbb3887dbc639530cf76bff50142e86d11c79dc99d5ac45cc07d`, actual LF
`31739c3dd193c48b1656f83c72208df66bf3453ee42c9578bdbc660639c07e4b`.
Synthetic CRLF encoding reproduces the declaration; private physical checkout
bytes are not read and no behavior defect is inferred from this alias.

The exact current-base two-field patch is `patches/ZONE_PROFILE_ADAPTER_SOURCE_PIN_REFRESH_2026-10-11.patch`, SHA256
`d2d43bf194d2b943a650c889e90482914b04ffc5bdb5481d838803eac1fb38d7`. Only `candidate_worktree_evidence.source_pins.tests/run_test_entry.py`
in writers.json and writer_coverage_matrix.json changes. All 503 other values,
new Zone/manifest pins, policies and source inputs remain. Apply/reverse checks
exit 0; semantic inverse restores the complete original JSON; candidate hashes
are 504/504 exact. Primary adoption is unconfirmed. Earlier c72/221f context
patches and receipts remain immutable at their original source; this context
supersedes them for the new base. There is no native/validator/generator/matrix
or service job for the metadata patch.

Separate packet `D:/Dev/Tests/Duris/accounting-accel-integration/ACC-11-ZONE-NATIVE-PROFILE-01-pin-review`; qualification SHA256
`1959adedfdbf26c33fd2af369f7a7affd24986fc37a4637135fc1d06eed92306`;
13-member seal SHA256 `7de6ca1d3424383e8b787634c3f8c88ea98f204e7001aaa9796f5bea145f91ed`.

Two local preparation failures are preserved: an unmatched lowercase generator
token prevented cohort creation, then a broad numeric substitution changed a
SHA substring and failed the metadata token check before any candidate write.
The corrected metadata helper starts from the original exact SHA constants;
the frozen source and independent native inputs were never changed. These are
preparation failures, not failed native assertions. The earlier wrong WORLD
documentation-path lookup is retained in the chat tool transcript; the actual
published document is ZONE_NATIVE_THREAD_SOURCE_PROFILE_INTEGRATION_2026-10-11.md.

## Ownership, cleanup and retained limits

Coordinator `01a129b3-9c1a-77b3-a10a-3ddafdb5c0a0` owns job `ACC-11-ZONE-NATIVE-PROFILE-01`.
Evidence `D:/Dev/Tests/Duris/accounting-accel-integration/ACC-11-ZONE-NATIVE-PROFILE-01`; build and scratch use separate direct D: mounts.
Disposable QA `67e366b999fcf2ae2d0c4b7200302514565f3e2efdcb1666480ae84cb6291b20` uses pinned image
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`, CPU1/2 GiB, restart=no, no network and read-only source/helpers.
Exact CID/image/owner/job and all six direct D: binds are checked before start.
Admission: 4.700779 GiB free Windows RAM and
1521.345619 GiB free D:. Pooled build slots are absent,
no foreign cache is reused and only one heavy cohort is granted. The original
fixture transport's 64 MiB stack setting supplies no 32 MiB aggregate-host proof.

Owned QA is removed and the guarded exact lease RELEASED at
`2026-10-11T18:42:53.036610+00:00`. Fresh terminal review verifies exact owned
absence, zero owned running jobs, global heavy lock absent and the same seven
foreign service IDs. Four passive diagnostic volumes remain preserved without
consumers; no other owner's service or job is stopped.

Prior ACC09 Item/held qualification stays bound to c72, Utility ACC10 to221f,
and earlier failures retain97/7f sources. No original observers, Coordinator
caller/publication, runtime/native authority, transitive ABI/emitted stack,
32 MiB host bound, first-EOF reader, both-engine durability, retention/restore,
gameplay, Plans1-5/R1-R8 or release acceptance is inferred. Accounting inactive,
admission CLOSED, coverage incomplete and release BLOCKED. Helpers remain parked
under the direct reduced arrangement; no domain runtime lease is activated.

## Related follow-up observed after the native run

Published primary advanced to `60463c342047db437b88994670bffcbcbb646c9c`, tree
`20f9ce838e7e935fddfab92fd0a9750ef30feb7c`, parent7ad, with seven changed paths.
Only Coordinator production source changes: exact inverse verifies its native
thread guard, three-to-two handle term and related comments. All other provider
bytes and all931 writer policies/2911 census entries remain. One Coordinator
pin changes; the other503 values, Zone source and permanent regression inputs
remain byte-identical. This is source-only correspondence; no604 compilation,
private getter/caller execution or native acceptance is promoted from7ad.
The new private primary report is not an independently reviewed raw packet.

The same patch d2d43bf1 is additionally apply/reverse checked against both exact
604 metadata files. Candidate504/504 raw Git hashes match, semantic inverse
restores the original full JSON, and the other503 values include the newer
Coordinator pin. Primary adoption stays unconfirmed. No patch regeneration,
native/validator/matrix/container job or runtime lease accompanies this check.

Separate source-only packet:
`D:/Dev/Tests/Duris/accounting-accel-integration/ACC-11-ZONE-NATIVE-PROFILE-01-followup-source`.
Review SHA256 `7e680de1b9ad9062f83c56e4f2f5a6c5fac102676e339272f6c53975076f1975`;
17-member seal SHA256
`dab690f510afd0156535f669fd125306e25d62a9f871c20df5a749c460082376`.
The finite queue keeps current604 caller/native acceptance open; qualify a
meaningful executable delivery when available, without repeating the unchanged
Zone/Utility/Item/held gates per commit. Native ACC11 remains exclusively7ad.

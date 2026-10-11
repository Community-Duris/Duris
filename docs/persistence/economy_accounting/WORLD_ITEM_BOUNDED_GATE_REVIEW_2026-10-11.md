# WORLD review of the bounded Item gate

This independent read/hash/Git review confirms that ACC-08-ITEM-BOUNDED-01
passed its bounded Item executable but failed the original registered outcome
contract with `missing required cases: main`. The refusal remains valid under
the unchanged checker and required-case list. The accompanying portable patch
corrects two recorded source hashes only; it neither resolves that refusal nor
changes the actual default-validator passes. No primary adoption is claimed.

## Pinned packet and review scope

The reviewed primary source is `7f1d766c068501427c099360bb4d1985240395a4`,
tree `2ec2ee21a1be222719f4969ae5909f4677690b2f`, three commits after
`97ded8571375f3fdc6168487a13120f65caedbec`. The coordinator's helper candidate
is separately identified as `dcb6840553002620a09cf0015d484a527ace2e05`;
the native result belongs to the primary source pin, not that helper candidate.

Reviewer chat `01a129b4-a57a-76e0-baec-42b3dd2fe62f` received one bounded
review ticket. Its maintained checkout began clean at
`f7228f3fe733863784d8f9526efd2de4596d3000`. This standalone document is its
only repository change; its own review receipts are on D:. No native executable,
outcome checker, validator or matrix was rerun. No heavy lease, container,
database, primary source import or runtime implementation was started.

The original packet is under
`D:/Dev/Tests/Duris/accounting-accel-integration/ACC-08-ITEM-BOUNDED-01`.
All 4,022 sealed members, totaling 147,390,631 bytes, match their recorded hashes.
All 3,983 exported regular files match their Git modes, blob identities, bytes
and SHA256 values at the primary pin. The source archive contains exactly that
selected export. This does not assert that the export is the entire repository.

## Actual executable result and registered refusal

The original registered command was:

```text
python3 -B /opt/source/tests/run_test_entry.py /opt/source/tests/async/test_item_transfer_complete_bounded.py script /evidence/entry-cases.json 1
```

It returned exit 0 in 129.014212634 seconds, without timeout or stderr. Both
SQL-header and flat production-object recipes reported 15 successful groups,
2,653 refusal points and 2,653 reservations. Both reports also set lockpick
success, malformed binding and NQF1 13/14 checks true. All three required markers
were present, and the original checker recorded zero skipped checks.

The original case receipt is nevertheless:

```json
{"cases":[{"id":"script","status":"passed"}],"called":["main"],"complete":true}
```

This excerpt omits only elapsed/evidence/empty-collected fields. The row still
requires `script` and `main`, retains its 5,400-second timeout, and requires
`ITEM_TRANSFER_COMPLETE_CASES`, `item-transfer complete bounded sql ordinary: PASS`,
and `item-transfer complete bounded flat ordinary: PASS`.

Static inspection of original `tests/run_integration_matrix.py` lines 178-195
confirms that `outcome_contract` derives observed cases from `result.cases`.
It subtracts those IDs from the required list; `called` and `complete` do not
supply an observed passed case. Original `tests/run_test_entry.py` separately
observes the entry call and records the script outcome. The retained coordinator
helper constructed the real `tests/run_regression_tests.py::TestResult` from
these observations and invoked the original checker. Its sole retained refusal
is `missing required cases: main`; terminal status is
`REGISTERED_ITEM_OUTCOME_FAILED`, with outer Docker exit 1.

The adapter, matrix runner, regression runner and held-retirement policy test
match their predecessor Git bytes. No requirement, assertion, marker, timeout
or checker was weakened. The executable pass establishes bounded Item component
evidence only. SQL-header compilation does not establish a real SQL service,
and these results do not establish NPC admission/runtime, gameplay, journal/ACK,
durability, a full production build/matrix, architecture, activation or release.

## Source-pin mismatch and two-field portable patch

The original default validator returned exit 0 in 13.802406640 seconds with
14 fixtures, 931 writer routes, 2,911 candidate sites and `release_ready=False`.
Independent hash comparison still detects a stale manifest pin in both metadata
writers: declared `a1c78d916a646baf5e668365a877dc5bd83f5f9bec205237c4dfd9a63cc68741`,
actual `268f2921125a4b9b47607c28ed12dcf42ad8c4e29e08d6a29fbf1640946b4dc2`.
This mismatch is separate from the observed validator pass. All ten refreshed/new
source pins match the frozen Git bytes; 486 predecessor pin values are preserved.

The metadata candidate changes only
`candidate_worktree_evidence.source_pins.tests/integration_manifest.json`
in `docs/persistence/economy_accounting/writers.json` and
`docs/persistence/economy_accounting/writer_coverage_matrix.json`.
Structural JSON comparison and exact portable-patch reconstruction confirm those
two field changes; the remaining 3,981 exported files are byte-identical.
The row, case list, markers, timeouts, policies and runtime/test source remain intact.

Retained apply-check and reverse-check receipts both record exit 0. The retained
candidate default-validator receipt records exit 0 in 6.625 seconds with the same
fixture/route/site counts and `release_ready=False`. These commands were not
repeated by this reviewer. The patch repairs hash accuracy only; it does not
repair default-validator acceptance or remove the actual registered refusal.

## Recorded ownership and release

The sealed container inspection, grant, global lease snapshot, cleanup and release
receipts agree on coordinator owner `01a129b3-9c1a-77b3-a10a-3ddafdb5c0a0`,
job `ACC-08-ITEM-BOUNDED-01`, source pin and exact QA CID
`6f74aca168ca02ec39684c153e6ba4f1b2ad9a2c0e8113b834c80964e8d4f194`.
The image is `sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`.
Grant mounts match inspection, including read-only source/helpers and direct D:
build/scratch/evidence mounts. Inspection records one CPU, 2 GiB memory,
4 GiB combined memory/swap allowance, network none, restart no and auto-removal.

Cleanup records `OWNED_QA_REMOVED`; the separate release receipt records
`RELEASED` at `2026-10-11T15:51:38.573006+00:00`, owned QA absent and zero
unleased owned containers. Seven preserved service IDs and four passive-volume
identities match preparation. The original global snapshot is historical
`GRANTED` evidence; the later release receipt supplies the terminal status.
This review checks those retained ownership/release records; it takes no fresh
live Docker or global-lock inventory and makes no new resource-state claim.

## Receipt identities

Reviewer receipt root:
`D:/Dev/Tests/Duris/accounting-world/ACC-08-ITEM-BOUNDED-01-review`.
Portable patch and supplementary receipts are under the coordinator's
`ACC-08-ITEM-BOUNDED-01-metadata-review` evidence directory.

| Receipt | SHA256 |
| --- | --- |
| Original terminal seal | `eac76e0f3e36236942e608a08dd1d85a1f79e759eeb68d344dd96a8ae5103028` |
| Resource release | `b0416b5ad274818e9e26b37691df01d9ee22b312d67831938ab24c2315deebde` |
| Registered entry result | `1c8309645886871cd062280e11c25ae20753f7b05fc42236a2375a2bd0faf78c` |
| Original case receipt | `5840204b3c34125d22fa80433be99a463ea5d5417be4fc8d1f07358bf8f2bd75` |
| Original outcome refusal | `7c74a632b885f8f707ae9624dbfeef3e1f12e66eabb97172a00e4b70faf12c30` |
| Original default-validator result | `2f667cfdd85b0180db287f131c7778ec46c361f1fddb92bb1f4485642be0e3bf` |
| Portable pin-only patch | `7e5cb59e22224765c572d484ffe12b8993d9ccd797c5703a439f4e392df2eb3c` |
| Apply-check result | `a9ba442b8bf3fcf5daeca2fd3a20b58028746ca3be7c7367ca61cff6d974e8bf` |
| Reverse-check result | `92e4067565efe156237deb4b5b202354318e5daefafce0bc45b57fcd4940f2e2` |
| Candidate default-validator result | `c23a017ec3fca5a42a45206634ed3fb89975a713307d7f9fd13d14d39881ba06` |
| Independent WORLD review | `95d632998dbd7223a546096863a90c793b137afa360f7bcc05e8800faa7ac829` |

The bounded review is complete. The registered Item gate remains refused, broader
qualification remains open, and this reviewer returns to idle.

# Plan 5: inactive flatfile native deletion and retained history

Delivery branch: `codex/accounting-plan5`. Worktree:
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Base: `04fe859629fc7ea8e4b127b9a8e9039510c3c8f7`. The result is the commit
containing this report, recorded after commit in
`tmp/plan5/flat-retention-evidence.json` and published on the separate delivery
branch. No direct push to `experimental-accounting` is made.

Canonical remote refreshed before choosing this slice:
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. Native source is tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, all 1,232 native inputs,
including canonical migration 0056. Primary-owner unpublished integration is
not qualified by this branch.

## Qualification gap and owned extension

The existing real flatfile deletion journey covers ordinary character deletion
and whole-account deletion following durable and uncertain fence-publication
faults. It checks native snapshot/alias preservation during refusal, repaired
retry, exact deletion, cache publication and cold restart. It previously did
not compare retained economic evidence through those boundaries.

The owned Plan 5 observer now supports `--backend flatfile`, with an explicit
flatfile server and native deletion inspector. It reuses the unchanged native
journey for all three paths. The owned native retained-history fixture adds a
`retention` mode carrying PID 1, matching the target identity in that journey.
Existing fixture modes keep PID 11 and their previous decisions. No game
mutation, shared accounting contract, schema, coordinator, registry/matrix or
activation-owner file changes.

Owned files:

- `tests/async/run_plan5_retention_journeys.py`
- `tests/async/flatfile_restore_authority_fixture.cpp`
- this report

The SQL `run` and `native_fixture` function ASTs are unchanged from the base.
The CLI defaults to SQL as before. SQL native deletion evidence remains pinned
to the base's published retention slice; those four SQL journeys are not
rerun or claimed as qualification of this new file's flatfile branch.

## Pilot correction and independent read boundary

The first pilot seeded retained history immediately after character creation.
That timing invalidated an existing earlier test: its stray marker makes an
otherwise absent accounting store refuse admission because it has no valid
control. A valid retained control changes that premise. The pilot consequently
failed its expected refusal after one read-only capture. This was a harness
interaction, not evidence of a production mutation defect.

The complete observer seeds history at the existing reconnect immediately after
that initial absent-store admission test has passed and removed its marker.
The history is therefore present before the later catalog/refusal, deletion,
fence-publication fault, pending-journal crash and restart boundaries. It does
not claim financial-retention coverage for the earlier pre-seed admission test.
The original journey is unchanged. The failed pilot's source, frozen inputs,
log and first cut are preserved separately as superseded evidence.

Every capture opens the already-existing native `.critical-authority.lock`
read-only without following symlinks and takes a bounded shared flock. The
native writer uses this same lock exclusively. The observer verifies private,
single-link, owner-controlled retained files and compares their complete bytes,
modes, link counts and names. It invokes only the independent retained-record
checker from `qualify_flatfile_economic_records.h`, not native storage readers
or recovery. Files are inventoried before and after that check and compared
exactly with the prior capture. The descriptor is closed in all cases.

The independent C++ reader is compiled with strict warnings, ASan/UBSan and no
PIE. It imports no mutation or native journal-recovery implementation. The
native seeded-history fixture uses existing test-only native staging helpers,
also compiled with sanitizers. Fixture creation is separate from the observer;
neither observer nor audit finding performs a correction.

The observer also verifies an all-zero active epoch, two retained epochs and
two source-claim files. It records whether the unrelated native deletion
authority journal is pending. The independent check never drains that journal;
the existing native server owns its recovery on the next boot.

## Retained evidence and actual journey scope

Each fresh private native filesystem contains 269 retained economic files:
lineage/control, epochs, native locator indexes, account mapping/lifetime rows,
initialized operation indexes, canonical operation segment and source claims.
There are six mapping lifetimes, including a retired and recreated wallet for
PID 1, and four historical roots across two epochs: two committed roots with
claims and two claimless rejections. Encoded command, EAI1/EAP1 plan, result and
durable receipt data are retained byte-for-byte. The rejected retry retains its
native source relationship to the successful root.

This is seeded nonzero history. The fixture's before/after currency vectors and
bank locator are synthetic; they are not produced by the player's actual
currency commands or claimed to match the live game balance. The synthetic
bank name is unrelated to the deleted account. Native player snapshots and
aliases are exercised by the existing real menu journey, while the independently
checked financial history provides retention evidence. This does not establish
complete capture, writer admission, typed active erasure or complete personal
data erasure.

| Actual native path | Consistent captures | Cold restarts | Retained files | Result |
| --- | --- | --- | --- | --- |
| Ordinary character deletion after missing-authority refusal and repaired retry | 3 | 1 | 269 exact | PASS |
| Whole-account deletion after durable fence-publication fault, pending-journal crash and recovery | 5 | 3 | 269 exact | PASS |
| Whole-account deletion after uncertain fence-publication fault, pending-journal crash and recovery | 5 | 3 | 269 exact | PASS |

The observer passes thirteen cuts and seven cold restarts. Both account paths
include a capture with a pending native authority journal before boot, with
retained history unchanged and the reader performing no recovery. The existing
journey passes its exact deletion, alias, usable account/refusal and cache
assertions. No active epoch is selected in any native menu run.

## Commands, tested source and evidence

Native jobs use `duris-plan5-origin-sql-tools:local`, immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with Ubuntu 24.04, Python 3.12.3 and GCC 13.3.0. The checkout is mounted
read-only and only `bin/` writable; private runtime states use native temporary
filesystems. No ports are published, capabilities added, production accessed
or checkout credentials used.

Common container invocation:

```text
docker run --rm --mount type=bind,source=<worktree>,target=/workspace,readonly --mount type=bind,source=<worktree>/bin,target=/workspace/bin --workdir /workspace <environment flags> duris-plan5-origin-sql-tools:local <command>
```

| Command | Result | Evidence under `tmp/plan5/` |
| --- | --- | --- |
| `DURIS_TEST_SANITIZERS=1 python3 -u tests/async/test_flatfile_character_delete.py` | PASS native character/account deletion, all 18 journal boundaries, active/corrupt refusals and paused legacy behavior | `flat-retention-inspector-build.log` |
| `DURIS_RUN_PLAN5_RETENTION_INTEGRATION=1 python3 -u tests/async/run_plan5_retention_journeys.py --backend flatfile --server /workspace/bin/server/dms_restore_flatfile --inspector /workspace/bin/tests/flatfile-character-delete-inspector` | PASS three actual native journeys, thirteen independent captures, seven cold restarts | `flat-retention-journeys-qualified.log` |
| `python3 -u tests/async/test_flatfile_restore_economic_authority.py` | PASS 18 valid stores, 317 corruption refusals, 670 native verifier invocations and 335 sanitizer-reader checks; evidence unchanged | `flat-retention-authority-regression.log` |
| `make -C src -j2` | PASS maintained SQL build check; native source and existing binary unchanged | `flat-retention-sql-build-check.log` |
| `./scripts/format.sh --check --file tests/async/flatfile_restore_authority_fixture.cpp` through WSL clang-format 14 | PASS | `flat-retention-format-check.log` |
| Host `python -m py_compile tests/async/run_plan5_retention_journeys.py` and `git diff --check` | PASS | Command output |
| Host invocation without the explicit integration gate | Expected exit 1 before native/database work | `flat-retention-gate-disabled.log` |
| Post-commit evidence recorder | PASS committed blobs, native inputs, helpers, binaries, frozen inputs and preserved prior artifacts | `flat-retention-evidence.json` |
| Host `python scripts/validate_economy_accounting.py --release` | REFUSED, exit 1: `writer has no executable evidence` | `flat-retention-release-validator.log` |

Qualified runner SHA-256:
`d770947940fec4822982efddab8bf34d382d99faf36d3a3ef864668c2381728c`.
Native history fixture source SHA-256:
`a3183ef5fdbc055366dfe103d2122c11196e25db1bbe513c51b83a020313a45a`.
These inputs are frozen before the qualified run. The manifest pins exact
compiled fixture/reader/inspector hashes, every retained-cut manifest, native
source, migrations, image and unchanged helper inputs. Previous Plan 5 binaries
and evidence remain preserved. Logs, binaries, fixture data and credentials
are not committed.

The existing flatfile server is reused from the recorded native build, SHA-256
`b39588923aa29ee4850b94c7c73d99c7eac1e6ce83ea45edf28e3d4854dab756`.
All native inputs match that build provenance; a new full flatfile server build
is not claimed. The SQL build check preserves binary SHA-256
`ac7646a258aa5ebb5a05a95c51f93797ad95e97d74716b3b5a05042910425ace`.

## Owner handoff, skips and completion gates

No interface or schema change is requested. The primary coordinator owner
should add the explicit flatfile invocation above after building its exact
candidate's flatfile server and native deletion inspector. Both account fault
modes and the ordinary character path are required. Registration is not edited
independently here. Existing SQL registration remains as described in the
previous slice.

This closes the inactive native flatfile retention evidence gap for the stated
history shape. Complete independent native capture, real supported economic
writer/UID lifetimes, typed active deletion/erasure, enabled export/governance,
controller-approved retention horizon, populated SQL upgrades, supported-route
gameplay/fault/replay, mixed workloads, numeric release-host budgets and the
primary owner's tested combined candidate remain gates. The retained-baseline
marker and metadata-header handoffs still belong to the primary contract owner.
The fresh release-validator refusal is an unresolved primary-owned coverage
gate, not a waived failure or an instruction to alter the shared registry here.
Full backup/restore and unchanged broad audit suites are not repeated here.

The required notebook/curator reconciliation remains pending: `AI_CONTEXT.md`,
the notebook reference and curator workflow are unavailable, and the existing
input request has no answer. This evidence report does not claim a notebook
update. Accounting stays inactive; wallet-root item exclusions, the declined
inactive spell change and active blackjack refusal remain preserved. No merge,
deployment, production mutation, activation or release completion is performed.

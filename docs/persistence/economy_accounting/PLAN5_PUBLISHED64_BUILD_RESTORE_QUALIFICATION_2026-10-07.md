# Plan 5 published canonical 64 build and restore qualification — 2026-10-07

Both original production profiles compile and link all754 units from the current
published native source, with zero warnings/errors and no reused objects. The
managed restore module runs all12 original cases: eleven pass, including both
real database engines; its lifecycle case fails at the stable online audit of
a source carrying pending recovery evidence. The already-published owned fixture
fix then passes that lifecycle class against the same fresh native source. These
are separate source qualifications, not a joint candidate or full R7/R8/release
pass. Native source capture, lifecycle installation and activation remain unrun.

## Branch, exact sources and ownership

Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Local and sole remote publication branch: `codex/accounting-plan5`.
Base: `a5dac7db92e4e251145f0eb7f2a030f015d8a246`. The containing commit is
the report result; the post-commit delivery receipt binds its exact SHA, remote,
clean worktree and all seven earlier branch tips as ancestors. No branch switch
or edit to another checkout occurred.

The new commit owns only this report and the additive remote follow-up document.
No native source, migration, registry/matrix, coordinator, shared runner or
activation file changes. No schema or public API fields are requested.

| Executed source identity | Exact value |
| --- | --- |
| Published primary commit | `62d030746265067638155314b3165bb135cb2c80` |
| Published complete tree | `743ef1430191a06b9f83bdbebb178856a5c515d1` |
| Native tree for every executed candidate | `833d3085815b396861ad18a77635412212381e4b` |
| Migration tree for every executed candidate | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Canonical head | `0064_auction_custody_history` |
| Published source archive SHA256 | `972771ab4666a6849e7773f068905ad7d4066b3b0365acc6f88acaa33ee5a71e` |
| Lifecycle-only composed tree | `f2f82cab4415946f5917c9b385c941949e86962e` |
| Composed archive SHA256 | `dd1fabf3263fe5bd8f58469e311e5503b091ec07031205d0b69d2d50cc810b2c` |
| Sole composed owned test blob | `d1ef31c19e79228e994e5c764a8553875db0f86e` |

The composed archive takes exactly one file,
`tests/async/test_persistence_backup_integration.py`, from the base branch.
Its existing pending-source fix is
`4f277dcf806ecca667e141ac8628bb06cf0f8e71`; previous owned additions in that
file remain intact. Only `FlatfileLifecycleRecoveryIntegration` runs in this
follow-up. The eleven successful public-source cases are not rerun. The file's
additional SQL claim journeys are not executed or qualified by this follow-up.

Both archives authenticate every6452 regular body and four symlink targets
against raw Git blob identity: no omitted members or transport conversions.
Terminal native guards preserve all archive bodies, canonical regular modes
(6093 files at0664 and359 at0775) and link targets. The later refresh
`dedad4e51d6a11a8b28be2bb380e4be5204e9b2e` has identical native/migration trees;
its latest checkpoint, requirements and Plan5 documents are retained as raw bytes.
`AI_CONTEXT.md` is absent at that published revision. Private combined source,
provider union and native lifecycleV2 source are not imported or executed here.

## Original recipes, complete provider proposal and primary handoff

The untouched original `build_restore_qualifier.build` and `_restore_fixture.build`
both fail to link. `item_transfer_command.c` retains references to lockpick
retirement validation, quest-cost projection encode/decode and quest coin-give
projection encode/decode. The real definitions already exist in the native tree;
the source lists omit them. Original link errors, full argv and source guards
remain in `probe/`, exit1 after136.308325 seconds.

The isolated proposal appends these real files exactly once at compilation:

- `src/item/lockpick_retirement_continuation.c`.
- `src/economy/native_quest_cost.c`.
- `src/economy/native_quest_coin_give.c`.

Both recipes then link, exit0 after142.987078 seconds. Full original provider
lists, flags, warnings-as-errors, section/link controls and test bodies remain.
The proposal also supplies the same required closure to the native authority and
lifecycle fixtures used by the actual managed journeys. Complete original and
proposed argv are retained in each `commands.json`; there are no replacement
stubs or omitted providers.

Primary-owned application request:

1. Append the three basenames to `scripts/build_restore_qualifier.py:SOURCES`.
   `_restore_fixture.py` inherits that list; avoid a second direct addition.
2. Append the three full paths to
   `tests/async/test_flatfile_accounting_store.py:SOURCES`. The lifecycle and
   authority fixture builders inherit that shared list. Their real proposed
   compiles and executed journeys pass here; the original accounting-store
   fault/crash main is not executed here and remains required after application.
3. Integrate the existing owned pending-source fixture fix and its input-family
   binding, preserving all prior tests. Keep the stable online audit before
   seeding pending evidence, then explicitly require the pending-source audit
   to refuse without modifying source bytes. Preserve recovery only in the
   manager's fresh isolated candidate. Run the whole selected combined source
   after these shared applications; separate results below do not replace it.

Fields changed: source-list entries only. API/schema changes: none. Invariants:
every real provider exactly once; original flags, assertions, faults and budgets
preserved; inactive and declined spell behavior preserved; no source audit
recovery or automatic correction. Shared application/registration and primary
authentication are unclaimed.

## Native builds, original commands and backend results

Pinned read-only tools image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Actual tools report GCC13.3.0, C++20, Python3.12.3 and GNU Make4.3.
Each job has2 CPUs/3GiB, no host network, fresh RAM-backed `/workspace` and
`/tmp`, and a direct D: evidence mount. Regression cache is off. The restore
jobs use the original required SYS_ADMIN capability and unconfined seccomp for
their private namespaces. No existing runtime, environment file or database is
mounted. Database daemons have new private datadirs/Unix sockets with TCP off.

Both exact Make commands are:

```text
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=mariadb BIN_ROOT=/workspace/bin/tests/published62-native/sql/bin OBJDIR=/workspace/bin/tests/published62-native/sql/objects DMS_BINARY=/workspace/bin/tests/published62-native/sql/server
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/workspace/bin/tests/published62-native/flatfile/bin OBJDIR=/workspace/bin/tests/published62-native/flatfile/objects DMS_BINARY=/workspace/bin/tests/published62-native/flatfile/server
```

| Complete production profile | Result | Actual dependency closure | Server SHA256 |
| --- | --- | --- | --- |
| SQL/MariaDB | PASS;754 fresh objects/.d; zero warnings/errors |1346 inputs:1318 repository/28 pinned-image headers | `1a7e305e8be3f516ae6778d6eb03fadcb14859e0876df6d279fc1d4bc75d937b` |
| Flatfile | PASS;754 fresh objects/.d; zero warnings/errors |1340 inputs:1320 repository/20 pinned-image headers | `9462829d443f5e138feac60c30bb27c8ec81b921fadfb2862be4a5386e3b54be` |

Every repository dependency body matches the source archive. Actual image-header
hashes remain bound to the immutable read-only image; they are distinguished
from repository inputs. All objects, dependency files, full link logs and both
server bodies are retained. Make/native guard phases report329.290538 and
291.003031 seconds before artifact retention; exact Make stopwatch values remain
in `commands.json`:327.495739 and289.602638 seconds respectively.

Actual host commands, all in the unchanged Plan5 worktree:

```text
python -X utf8 tmp/plan5/run-published62-native.py probe
python -X utf8 tmp/plan5/run-published62-native.py probe-proposal
python -X utf8 tmp/plan5/run-published62-native.py build-sql
python -X utf8 tmp/plan5/run-published62-native.py build-flatfile
python -X utf8 tmp/plan5/run-published62-native.py contracts
python -X utf8 tmp/plan5/run-published62-native.py integration
python -X utf8 tmp/plan5/run-published62-native.py lifecycle-owned
```

Each stage retains its exact executed launcher, observer and Docker argv.
The restore observer uses the standard unittest loader and TextTestRunner on
the unchanged original module, with both explicit integration gates enabled and
the two hash-bound fresh servers. Its follow-up loads only the original failed
lifecycle class from the composed archive. The same test assertions and existing
60/120-second startup/cold-boot budgets remain. Observers record results and copy
evidence before cleanup; they do not replace checks or alter audit findings.

| Native managed execution | Result |
| --- | --- |
| Original full module/public source/provider proposal |12 cases;11 PASS/1 ERROR; zero skips;541.098s; overall FAIL/exit1 |
| Failed lifecycle class/composed owned fix/provider proposal |1 PASS; zero failures/errors/skips;243.526s; exit0 |

The eleven original passes include corrupt/unreplayable WAL quarantine, first
snapshot from WAL, missing economic record refusals before boot, real pending
WAL/transaction replay, exactly-once interrupted bank/legacy recovery, private
foreign-owned checkout boot, locker/spell receipts, both database engines and
checksum-valid corrupt lazy catalogs. All original cases remain in the module.
The original error traceback identifies the pending online source audit, not a
restore or database failure.

Original execution records nine accepted isolated service boots, eight accepted
restores among24 attempts, and40 database qualification cuts:24 accepted and16
deliberate refusals. Actual MariaDB10.11.14 and MySQL8.0.46 each bootstrap/adopt/run
the canonical head, dump, import into a separate daemon, qualify schema/history/
values and boot the actual SQL server. Original account relation, future money/
epic revision, cancelling-history loss, corrupted opening and migration checksum
controls remain. This is fresh canonical restore evidence; legacy upgrade and
rerun qualification are still required.

The passing lifecycle case performs two actual flatfile boots, drains pending
native journals/transaction evidence, preserves the old inactive receipt and
advances the native UID allocator/witness exactly from `(1000203,2)` to
`(2000203,3)` on the second boot. It retains two generations, prunes the unretained
old generation and refuses manifest loss/corruption, checksum-valid corrupt
receipts and required receipt loss before capture/boot. Original live sources,
journals and captured generation bodies remain unchanged. This remains modeled
known-native origins, with source capture/installation and fullR8 explicitly false.

Normal accounting validation, matrix check and runtime validation pass:
14 fixtures/926 routes,2900 occurrences/2842 mapped sites and canonical64/230
tables. `validate_economy_accounting.py --release` still exits1 for
`writer has no executable evidence`. No writer or coverage gate is promoted.

## Retention, limitations and remaining gates

Evidence root:
`D:/CodexEvidence/accounting-plan5/bin/published62-native-01-20261007/`.
Seal `seal/evidence.json`, SHA256
`8feed72db9055e652c30030ca05d5dcfe41bfe91cd6a7fde8773736b6e28dd6b`,
binds9924 entries/4,815,335,745 bytes. It verifies6700 regular bodies from47
native inventories observed before copying. Full original failures, accepted
and refused restores, generated states/journals, generations/dumps, receipts,
first/second service logs and cold-restart delta are retained. The metadata-only
seal preparation correction is retained without rerunning any native case.

Source modes/link targets were verified directly on the native filesystem.
`native-artifacts.json` build modes describe the copied D: bind view; original
RAM object modes were not retained. Pre-copy state inventories preserve native
mode/nlink/inode/device/mtime observations independently of that copied view.
No reparse entries occur in the retained loose evidence.

All seven native client/helper invocations closed with the reported exit codes.
Docker's API subsequently became unavailable during final container-state
inspection. That failed observation is retained; no service restart, disk/volume
move, native rerun or claim of inspected Docker/OOM state follows. Final container
state inspection remains unverified. It does not erase completed native results.

Existing checkouts, active build/job locations and Docker storage were preserved
when the new D: preferences arrived. Existing jobs used RAM scratch and retained
directly on D:. New seal helpers use `D:/Dev/Temp/accounting-plan5-published62`;
future builds use task-specific `D:/Dev/Builds/.../bin`, respecting original
native permission, short Unix-socket and workspace/bin requirements.

Remaining gates: primary-owned source-list application and full combined rerun;
private complete nativeV2/candidate/fixture export and authentication; all native
holdings/history/treasury owners; source-complete producer and actual-player
journeys; original fault/reference/load budgets; both-engine legacy upgrade/rerun;
authentic complete retention/restore policy qualification; and fullR1-R8/release.
The composed file's new SQL claim journeys are unrun here. No old0055, inventory,
synthetic-origin or separate-source pass establishes combined completion.

Accounting stays inactive; wallet-root item exclusions and declined inactive
spell behavior remain. No activation, audit correction, production mutation,
deployment or merge occurs. Report/follow-up/seal/delivery are curator-ready;
the primary-local notebook is nonblocking and its application/acknowledgement
and cross-chat notification are unclaimed. Full Plan5 remains active.

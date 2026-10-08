# Plan 5 native custody audits and saved-ground fixture repair

2026-10-08. Eight original native custody audit selections now pass on fresh
MySQL/MariaDB qualification instances. The saved-ground method first exposed
two stale diagnostic-schema statements; both are fixed without changing its
assertions, native probe or production code. This is component audit evidence,
not gameplay/producer, full native recovery, activation or release completion.

## Branch, ownership and exact inputs

Sole local/remote branch `codex/accounting-plan5`; worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Owned base `4c2c05d8e86e704e053b145aae072393c67837fa`. Result is this containing commit;
`D:/Dev/Tests/Duris/accounting-plan5/native-custody-20261008/delivery/result.json` binds the exact result/remote,
clean tree, all seven preserved historical tips and all 27 resulting overlays.
The primary was refreshed before choosing published base
`b55c688ec1790dbd86228ac1188bc579a543d474`. No primary/shared branch is pushed.

| Execution | Composed tree | Archive SHA-256 |
| --- | --- | --- |
| Original eight selections; seven PASS, saved-ground ERROR | `b4c3c8e14783a76a8e492579064044f158530d6f` | `20c78a69f825e678cd3f929d27bb497fc486e5da9d4da8571cadb350af351d05` |
| First correction; saved-ground full module 10 PASS, one ERROR | `adcb1cc767e1d49fd22980964b885ffe774599b0` | `79bbcdc7a8f28b7b3ccb9c3aeb577ee73259ef52013728d603bce7b7017cba5a` |
| Complete correction; saved-ground full module 11 PASS | `98225ac8a49f7b08cafa71c478156d69a984dd5e` | `2020525332bc9748334ffedb436b05cd53367ae5de1aa53f1d1d8c0cd2f6f50f` |

Native tree stays `833d3085815b396861ad18a77635412212381e4b`; migration tree stays
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`, canonical head 64. These are current 64
checks, not historical 0055 qualification. Each run authenticates 6,517 regular
source bodies/modes and four symlink targets before and after execution.
All common product/native/schema inputs and 26 other overlay blobs are exact;
the only source difference is `test_sql_saved_ground_custody_audit.py`.
Seven original native passes retain their source00 scope; the final full saved
module uses source02. No single combined 18-test invocation is claimed.

Owned changes are that test file, this report, and an additive remote notebook
follow-up. No shared contract, coordinator, producer, native implementation,
migration, registry, matrix, activation owner or shared build recipe changes.

## Established defect and complete fix

The original saved-ground method constructs its diagnostic marker schema by
executing `run_economic_sql_audit_snapshot_mysql.TABLES`, which already defines
`item_owner_revision`. It then creates that table again, causing MariaDB error
1050 before marker predicate checks. The unchanged run's traceback and original
preimage remain under `custodyB00` and `saved-ground-before.py`.

The first correction removed that duplicate creation. The full module then
passed all ten pure methods but exposed MariaDB error 1136: its immutable-payload
marker still used `INSERT INTO sql_room_item_payload VALUES (81)` although the
same diagnostic table now has six columns. This intermediate failure is retained
under `custodySaved01` and its exact source01 archive.

The complete fix removes the duplicate creation and names the intended
`item_uid` column in that marker insert. It reuses the existing diagnostic schema
and preserves every marker value, label, expected outcome, readonly inventory
check and explicit synthetic-authority limit. There is no `IF NOT EXISTS`
workaround, schema downgrade or relaxed assertion. AST binding proves every
existing `assert*` call identical. The original generated C++ body and complete
compiler argv are identical across all three saved-ground attempts.

The existing full integration method is the regression test. No redundant new
test is added. The final entire module passes **11 methods, zero skips**, including
both native/SQL engine paths and every original marker branch.

## Executed original checks and read-only evidence

| Custody check | Observations | MariaDB / MySQL | Stage |
| --- | ---: | ---: | --- |
| auction | 14 | 7 / 7 | custodyA00 |
| corpse | 68 | 34 / 34 | custodyA00 |
| locker | 66 | 33 / 33 | custodyB00 |
| player | 46 | 23 / 23 | custodyA00 |
| room | 34 | 17 / 17 | custodyB00 |
| saved ground | 164 | 82 / 82 | custodySaved02 |
| shop | 38 | 19 / 19 | custodyA00 |
| siege | 64 | 32 / 32 | custodyB00 |

There are **494 recorded observations**, each with unchanged authority before
and after the audit/capture step. The eight successful native cases use 16 fresh
engine sessions. Two failed saved-ground attempts add two more isolated sessions;
all 18 private daemons terminate. All four test containers finish without OOM and
with no recorded child process left live. Original `private_database` owns only
new task-private instances; existing services/volumes are untouched.

The original fixtures exercise native capture/parsing, physical custody and
competing sources, exact UID/owner/root/parent/equipment/coin representations,
retained/tombstoned history, damaged/orphan metadata, aliases and output-limit
invariance. Python reader accounts have SELECT-only permissions and readonly
transactions; fixture owners alone set up or deliberately damage the disposable
data. Native capture checks retain their original credentials and protocol.
Before/after authority inventories detect any unexpected mutation.

Saved-ground retains 30 explicitly synthetic marker-only observations within its
164 observations. Their expected history-presence fences never become native
admission/recovery authority. Room records 34 observations, including four
canonical foreign-key refusals; those rejected fixture changes do not masquerade
as four independently executed damaged audit cuts. Other rows also remain
fixture observations, not actual gameplay or source capture installation.

Successful qualification comprises seven unchanged original native methods and
the 11-method final saved module: **18 distinct methods, zero skips in those
successful runs**. Repeated ten pure passes in the intermediate failed module
are not counted again. Both original failed attempts remain failures in the raw
record; `custodyB00` and `custodySaved01` each finish exit1.

## Exact commands, backends and build scope

Host entrypoints, from the owned worktree, using host Python 3.12.10:

```powershell
& 'C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe' -X utf8 -B D:/Dev/Temp/accounting-plan5-native-custody/launch.py custodyA00
& 'C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe' -X utf8 -B D:/Dev/Temp/accounting-plan5-native-custody/launch.py custodyB00
& 'C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe' -X utf8 -B D:/Dev/Temp/accounting-plan5-native-custody/launch_fixed.py custodySaved01 01
& 'C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe' -X utf8 -B D:/Dev/Temp/accounting-plan5-native-custody/launch_fixed.py custodySaved02 02
```

The actual container entrypoints are `python3 -u -B /evidence/<stage>/observer.py
<stage> <00|01|02>`. `selections00.json`, `selections01.json`, `selections.json`
and per-method logs bind every exact unittest selection. Each original eight
opt-in uses `DURIS_RUN_SQL_<AUCTION|SHOP|PLAYER|CORPSE|LOCKER|SIEGE|SAVED|ROOM>_AUDIT=1`.
Both corrected saved runs load the entire `test_sql_saved_ground_custody_audit`
module. Each stage's `docker-command.json` preserves the complete executed argv.

Runtime image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Python 3.12.3; GCC 13.3; MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1 and
MySQL 8.0.46-0ubuntu0.22.04.4. `tools.json` binds actual version output, binary
SHA-256 and bytes. Two initial independent batches use separate D: bin/evidence
mounts; corrected runs have their own fresh bin mounts. Each test container has
network `none`, a read-only root, two CPUs, 5 GiB memory, private tmpfs source/temp
storage and a 2400-second observer deadline. Every stage retains original
ASan/UBSan, warning/error and link flags; existing garbage collection flags remain
exactly where the original recipe used them. Writable build caches are off.

Across all attempts: 10 fresh original fixture compile/link calls, zero compiler
queries, 18 `mysql_config` queries, 1,589 native test executable invocations,
2,916 external commands and2,952 recorded launches. External exits are 2,877 zero,
29 expected private-daemon startup probes exit1, six original CLI-finding checks
exit1 and four original unsigned-UID history refusals exit7. PyMySQL's two genuine
fixture errors are separately retained; they are not expected passes.

The full module's final test runner reports 88.718 seconds. Full observer stages:
A00 146.839767, B00 165.837684,
Saved01 58.604561, Saved02 90.035719 seconds.
No maintained server build/boot, live producer/player journey, flatfile persistence
journey, erasure adapter, activation or remote backup custody is executed here.
The room fixture's `__NO_MYSQL__` codec build is component evidence only.
Fresh bootstrap/adoption/current 64 history checks are not an older-release
upgrade or full migration/release acceptance claim.

## Evidence and remaining shared gates

Evidence: `D:/Dev/Tests/Duris/accounting-plan5/native-custody-20261008`. Build artifacts:
`D:/Dev/Builds/Duris/accounting-plan5-native-custody-20261008/<stage>/bin`.
`qualification.json` binds all stages, original failures, observations, native
binaries and source hashes. `qualified-cases/*-evidence.json` preserves the eight
successful original observation arrays. Each stage preserves raw commands,
launches, method logs, source authentication, build inventories and selected
regular temporary evidence. Database data bodies and private keys are excluded
from retained temporary copies; no link is followed by the evidence copier.

`source-change-and-gates.json` binds the one changed source path, identical
assertions/common inputs and remaining opt-in inventory.
`nonzero-command-classification.json` classifies every external nonzero exit.
Raw seal SHA-256: `bd1da577bcfe75417ee5e0585044882a111a6dbe0bcbd6afe52af85ad278db9b`. `seal.json` binds regular evidence/build bodies;
post-push `delivery/result.json` rehashes those files and binds committed blobs,
remote/clean state and preserved ancestry. Generated evidence is not committed.

Eight of the prior 17 explicit native/SQL opt-in selections now execute within
their stated scopes. **Nine other original selections remain**: native stake 1,
origin 3, canonical 4, child identity 1. Their exact names and source bodies are in
`source-change-and-gates.json` and `shared-gate-inputs`. Original canonical tests
still contain four head 62 assertions against published 64. Origin setup uses the
shared native fixture whose missing genuine providers are already established;
current child/canonical/stake recipes also need the shared owner's original
provider composition review. No unchanged known failed build is rerun and no
shared harness/recipe is repaired independently. The genuine-provider/head64
handoff and full original missing-evidence controls stay required.

The [erasure proof request](PLAN5_ERASURE_PROPAGATION_INTERFACE_2026-10-08.md)
remains: the synthetic in-memory filter supplies no durable six-source adapter;
both nonempty-ledger/generation guards stay. No new interface/schema change is
requested or installed in this slice. Primary's private combined Smith/reset
source is still unavailable for release execution. Actual producer/gameplay,
opening/cutover, full native recovery, both backend route coverage, retention/
remote custody, release-host workloads and combined Plan5/R1-R8/full release
remain unqualified. Inventory and isolated native/synthetic passes do not close
those requirements.

The additive follow-up is curator-ready; primary-local notebook upkeep remains
nonblocking. Application/import/acknowledgment are unclaimed. Accounting remains
inactive; wallet-root ITEM_MONEY exclusions and the declined inactive spell path
stay unchanged. No production access, autocorrection, shared/primary push,
cross-chat message, deployment or merge occurs.

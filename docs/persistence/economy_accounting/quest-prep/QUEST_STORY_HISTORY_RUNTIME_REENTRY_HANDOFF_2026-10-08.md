# Actual story runtime reentry component — 2026-10-08

**PASS: real flat-provider component execution**, in O1 and Og with ASan/UBSan.
After one precisely targeted post-rename directory-fsync EIO, the actual runtime
returns false and restores its prior memory, while a valid new S1 authority file
remains visible. Actual bootstrap reloads that retained S1 fact. Retrying identical
original fields/key under S2 conflicts without changing memory, file bytes or file
identity. This executes the reservation in the [source boundary](QUEST_STORY_HISTORY_RECOVERY_BOUNDARY_2026-10-08.md);
it does not change season policy or qualify native economic completion.

## Publication and pins

| Input | Exact identity |
|---|---|
| Prep base | `79105dcd50a05b9020ca7fd046b06b5d39f6ba00` |
| Executable code bundle, pushed | `406ab5ea1175a94a49d3203464023a2b0bfd606b` |
| Compiled public candidate | `abc763fc197fae4aa4609d21c00ae78d191d9bd2` |
| Prior research candidate | `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` |
| Final fresh-fetch observation | `5f5a8bdfd0306a6c65857cf936fd6b332832c90b`, docs only |
| All three candidates' complete `src` tree | `833d3085815b396861ad18a77635412212381e4b` |
| All three candidates' migrations tree | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Immutable source archive | 26,910,720 bytes; SHA256 `be81dcce45d740c21a3feb3988c599d607f35e13f05d8a9c5bddfd08571dff2b` |

The code commit adds exactly:

- `tests/async/quest_accounting_prep/story_history_runtime_reentry.cpp`
- `tests/async/quest_accounting_prep/test_story_history_runtime_reentry.py`

Its separate documentation companion adds only this handoff. Import the code
commit alone, then consume this document separately. No earlier optional helper
bundle is a dependency. Actual patch application to a standalone candidate source
export passes; imported test bytes equal committed bytes and the executed inputs.
No second runtime execution is inferred from that import/identity check.

| Executed/committed test | Git blob | SHA256 |
|---|---|---|
| C++ harness, 14,372 bytes | `a4e9cb1faac0bdde392752256a25b49ee9dffbef` | `32d5f359b9c56f3c4aa251feb67e071e8a5d11e0bcd49eecb86fffb41cd06e79` |
| Python runner, 13,066 bytes | `bed72dc1ac434ed4a0bc73c5a9b48891ec9ab73e` | `394c84d8affa8bcaf729e30297bf75e3deb1c5588d476023bb9c46cf5d62ada7` |

## Actual providers and fixture boundary

The runner exports the pinned **whole `src/` tree**, then compiles these complete
maintained translation units: `zone_story_quest_runtime.c`, `zone_story_quest_production.c`,
`zone_story_quest_feature.c`, `zone_story_quest_tracking.c`, `zone_story_quest_catalog.c`,
`flatfile_zone_story_quest_state.c`, `flatfile_store.c`, `flatfile_authority_transaction.c`,
`persistence_mode.c` and `flatfile_ip_activity_repository.c`. No function extraction,
replacement tracker, production parser, flat writer, lock or recovery implementation.
The real mode/root provider provisions the private root and resets its fixture IP
activity. Actual authority-lock/recover calls execute with no pending journal;
no journal commit scenario is introduced.

Only three SQL seams exist: `sql_season_epoch()` returns zero and counts calls,
selecting the actual runtime's configured-season fallback; SQL story load/save
abort if selected. Mode/root, production catalog, story serialization and file
validation are real. Function/data sections and linker garbage collection omit
uncalled character UI/legacy methods with server panic-handler dependencies;
the executed authoritative-completion path is retained. Symbol dumps and link
maps disclose that boundary. No non-SQL link stub is added.

Booted Q/index globals reuse the maintained production-catalog harness pattern:
giver VNUM17, zone1, one ITEM24402 input and ITEM24403 reward, no disappearance,
coin or fee. These are minimal **component values**, not native NPC births or
actual inventory handover. Production `bootstrap` constructs and binds this Q;
runtime `bootstrap` creates the actual service and loads real flat authority.
Dispatch is a direct call to actual `record_authoritative_completion`, not
`quest.c` gameplay, full world boot, AREA loading or reset dispatch.

Original key is `runtime-reentry-original`; ordered credited PIDs701,702;
direct PID701; room101; time1791400000; content revision1; S1=101 and S2=102.
Actor metadata is `ComponentActor`, level20, racewar1, known party size2 with
strongest level25; daily policy is disabled. Production canonical contract is
`give=I:24402;receive=I:24403;disappear=0`, definition
`zone-story:qst:17:676976653d493a32343430323b726563656976653d493a32343430333b6469736170706561723d30`.
Expected T envelope uses the real transaction formatter with manually specified
fields. Actual bootstrap invokes the real parser. Separate retained-evidence
verification decodes the observed T envelope and checks every transaction field.

## Executed sequence and observations

| Step | Actual result/assertion |
|---|---|
| Healthy fresh root, S1 record | True; one exact original T; serialized memory equals validated authority. |
| Identical S1 replay | True; one T; complete serialized state and file bytes unchanged. Atomic rewrite may change inode. |
| Fresh failure root, original S1 save | Successful real target rename, then exactly one directory-fsync EIO; false return; error `sync authority directory: Input/output error`. |
| Immediate readback | Memory restored exactly to prior healthy `ZSQF\|1\n`; real provider validates checksum/envelope and returns the same S1 document/file bytes as the healthy control. Visible file inode equals the inode captured immediately after rename. |
| Actual bootstrap with S2 configured | True; memory equals retained S1 document; file bytes/inode unchanged. |
| Same original key and fields, S2 retry | False; exact error `completion transaction ID was reused with different data`; one original S1 T; no publication, second fact, byte or inode change. |
| S1 same-key control after reload | True; retained state/file bytes unchanged. |
| Distinct key under S2 | True; original S1 T preserved plus exact `runtime-reentry-distinct` S2 T; real new authority published. |

Both binaries report `epoch_calls=7 target_renames=5 injections=1`.
The hook calls the real `renameat` first, matches both directory fds by exact
device/inode and target filename, captures the published regular-file identity,
then fails only its next matching directory `fsync`, after rechecking the file
identity. Other rename/sync calls pass through. Injection disarms immediately.
The complete original T content, complete serialized documents, raw file bytes,
and device/inode/mode/UID/size/counters are retained per observation.

## Reproduction and evidence

Executed from the existing prep worktree in Ubuntu-22.04 WSL:

```powershell
wsl.exe -d Ubuntu-22.04 --cd '/mnt/c/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max' --exec env TMPDIR=/mnt/d/Dev/Temp PYTHONDONTWRITEBYTECODE=1 python3 -B tests/async/quest_accounting_prep/test_story_history_runtime_reentry.py --candidate abc763fc197fae4aa4609d21c00ae78d191d9bd2 --git-dir '/mnt/c/Users/alexa/OneDrive/Documents/ChatGPT/NewDuris Max/.git/worktrees/NewDuris-Max8'
wsl.exe -d Ubuntu-22.04 --cd '/mnt/c/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max' --exec bash scripts/format.sh --check --file tests/async/quest_accounting_prep/story_history_runtime_reentry.cpp
wsl.exe -d Ubuntu-22.04 --exec env TMPDIR=/mnt/d/Dev/Temp PYTHONDONTWRITEBYTECODE=1 python3 -B /mnt/d/Dev/Temp/quest-story-runtime-publication-20261008/verify.py
python -B D:\Dev\Temp\quest-story-runtime-publication-20261008\publish.py
git diff --check
git push origin codex/accounting-quest-prep
```

`--git-dir` only supplies this Windows-managed worktree's mapped metadata path;
ordinary Linux checkouts omit it. The public runner creates unique D: task folders
and accepts explicit evidence/bin/authority parents. The final run retains every
command, stdout/stderr/result and timeout. Deadline600s overall,90s per compile/link
and30s per component execution; a timeout kills the subprocess group.

Strict flags: C++20, `-Wall -Wextra -Wpedantic -Werror`, maintained client-free/test
defines and includes, `-ffunction-sections -fdata-sections`; O1 control and
`-Og -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all -fno-pie -no-pie`.
Links use `-lcrypto -pthread`, garbage collection and only `renameat`/`fsync`
wrappers. ASan leak checking and halt-on-error, UBSan halt-on-error/stack traces
are enabled. Compiler is Ubuntu g++11.4.0. All22 translation-unit compilations,
both links and both runtime executions exit0; no sanitizer findings. Runtime
durations are0.119638s/O1 and0.122000s/Og. Cross-profile state/file bytes match.

| Binary | Bytes | SHA256 |
|---|---:|---|
| O1 | 279,848 | `15d9e99f95f53f8b49d5c80220a9f40743c63965f80ead96b022f5b8d9d823f3` |
| Og ASan/UBSan | 13,887,440 | `c9af06927c179f5f913759b4681671bddc634e7c0c1fb16aca07c181ff6ed5c6` |

Final evidence: `D:\Dev\Temp\quest-story-runtime-6x5cuwrv`.
Actual builds/objects/dependencies/maps/binaries:
`D:\Dev\Builds\NewDuris\quest-story-runtime\quest-story-runtime-6x5cuwrv\bin`.
Additional verification/import receipts:
`D:\Dev\Temp\quest-story-runtime-publication-20261008`.
Whole source archive/file modes/Git blobs authenticate all1,334 source files;
all468 compiler dependencies including system headers, tool executables and
their dependent libraries, binary libraries, test inputs, build artifacts and
1,606 original evidence entries authenticate. Committed-input equivalence and
standalone patch SHA256 `c169f0c0f39d43d290c2794cb383c34527a6b939ccb3003fe9c6dc06622c0293`
are in `publication.json`. No evidence/binary/generated state is committed.

Retained unsuccessful attempts:

- `quest-story-runtime-8thh4gf6`: Linux Git cannot follow Windows `.git` paths;
  candidate lookup exits128, before compilation.
- `quest-story-runtime-8utny6gn`: disabled WSL executable interoperability rejects
  attempted `git.exe`, `Exec format error`, before compilation.
- `quest-story-runtime-t8zpvhfx`: all11 O1 inputs compile, link exits1 on uncalled
  UI/legacy panic-handler references. This exploratory attempt was superseded;
  only the final run's immutable copied inputs are claimed as exact execution.
- Initial standalone Windows patch import converts new files to CRLF outside
  the configured worktree, failing exact-byte comparison. That import remains;
  `failed-crlf-import.json` records its hashes. A fresh standalone import with
  invocation-local `core.autocrlf=false` passes. No global configuration changed.

## Limits and remaining owner work

D: is a9p/DrvFS mount without Unix metadata; the actual chmod0700 probe stays0777.
The maintained private-directory checks are preserved. Authority uses private0700,
UID1000 directories in existing `/dev/shm` tmpfs, with actual story files0600;
mount and metadata evidence is retained. Both profile roots are confirmed removed.
Tmpfs fsync/readback plus same-process bootstrap proves no physical-media or
power-loss durability, process restart, SQL behavior, native producer/source
custody, reward publication/movement, XP save receipt, obligation ACK or retirement.
This test owns story singleton observations only, no economic ownership decision.

The demonstrated false-save/visible-file outcome needs the shared owner's real
outcome/retry resolution when integrated with economic recovery. A retained
original key plus reconstruction under current S2 is observably a conflict;
choosing authoritative original-season policy or changing continuation formats
remains outside this bundle. Original native source/birth/custody, delayed
settlement/refund and native cost/publication/ACK/paired-retirement dependencies
remain as recorded in [the owner reassessment](OWNER_INTERFACE_REASSESSMENT_2026-10-07.md).
No shared file, existing test/driver, schema, registry, capture/decoder, finish
plan, canonical HANDOFF or Plan5 file changed. No server build/boot, database,
migration, native journey or broad batch ran. Integrated primary qualification
and the broader continuing native Goal remain unfinished/**BLOCKED**. This
finite optional component delivery does not resume or complete that Goal.

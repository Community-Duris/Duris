# Plan 5 bounded regular snapshot input — 2026-10-08

The independent saved-snapshot CLI's path-size check did not bound its actual
read. A FIFO without a writer blocked indefinitely after stat reported zero
bytes. An external writer could also grow a valid JSON file after stat, causing
an input larger than the existing 32 MiB cap to be read and reported healthy.
Both defects are reproduced on unchanged source before the fix.

The existing main now opens once with nonblocking mode where available, checks
that descriptor's regular-file type and declared size, and reads at most the cap
plus one byte through the same descriptor. Actual bytes above the cap refuse
before decoding, JSON parsing or audit. A finally block closes the descriptor on
all paths, including type/size/stream failures; the binary wrapper does not own it.
Strict UTF-8 decoding and the earlier duplicate-field rejection remain intact.
Raw input is released before audit. Malformed/over-limit input keeps exit 2 and
empty stdout. Nonregular descriptors use the fixed diagnostic
`reconciliation failed: snapshot requires a regular file`; over-limit input keeps
`reconciliation failed: snapshot or output limit exceeded`.

Accessible regular files, symlinks to regular files and hardlinks retain their
existing acceptance and views. This adds no owner/permission/link policy or
snapshot schema. No mutation implementation is imported, and input is never
repaired. An actual fixture writer deliberately grows selected test copies;
the reader leaves each resulting grown body unchanged. Bounded saved-file reads
do not establish a transactionally stable live capture or production release.

## Branch, ownership and interfaces

Sole local/remote branch codex/accounting-plan5, worktree
C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max.
Owned base 0a16ba2187ecfc77b6306c0e3f9a3be5d85eeb4e. The containing commit is the result. Its exact SHA,
remote equality, clean tree, seven preserved historical ancestor tips and sealed
body rehash are recorded after push in D:/Dev/Tests/Duris/accounting-plan5/snapshot-input-20261008/delivery/result.json.
No branch switch or push to experimental-accounting.

Four owned files: scripts/reconcile_economy_accounting.py,
tests/async/test_reconcile_economy_accounting.py, this report and an additive
PLAN5_REMOTE_FOLLOWUP_2026-10-06.md entry. Two standard-library imports and existing
main change; two focused methods are added to the existing test class. Every
prior other function/method, prior non-function AST apart from os/stat imports,
and the other 25 owned overlays are unchanged. Source00-to04 changes exactly the
two code/test paths. change-scope.json authenticates these claims.

Shared interface/schema requests: none. Coordinator, accounting contracts,
producers, writer registry/matrix, activation, migrations and original shared
native recipes stay with their primary owner. Preserve all 27 owned overlay
prerequisites during integration. Inactive behavior, wallet-root ITEM_MONEY
exclusions and the declined inactive spell path remain intact.

## Exact tested source and runtime

Tested published primary ab67bad7e079245128c4de04a82beed0df2a2e9e. Final primary plus 27 owned overlays
is c247140b0b7477d58737504e8ed8e243c14f1bca; archive SHA256 ae1dc082d7150ab47a52bbb5f72cdca877c598c3541125d18b2adc266d924613.
Published native tree 833d3085815b396861ad18a77635412212381e4b, migrations tree 7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2,
canonical head 64. Historical owned native source is not the native test base.
The bounded tests do not qualify every included overlay or the private candidate.

| Transport | Composed Git tree | Archive SHA256 |
| --- | --- | --- |
| 00 | 633d892e1dbf39354cfa963d30ee40c235a2f23d | 8ed7f393f862630907241cf07d09f73849d6d84b526ce6d0e9bb5a68d5b38590 |
| 01 | 386ba2e0f5809caa498a304934b2de02051db067 | aaed562f308001894ae3a1460f4a1494821915dd71b8dab5c468cf58d33831e6 |
| 02 | df83c04e1640321229704789b4d3c2c7fbf81be8 | 5118fc06814ae57d224c6479b1e268b63cc1674a83dd177c64bf5ab20e2d1b1c |
| 03 | 30dd973c467af473bcb7bd5bea5616b6d9108be5 | 65a5b0eed5ba1aa382f26cba9a508397539edf021f944093205d0fb3e676d091 |
| 04 | c247140b0b7477d58737504e8ed8e243c14f1bca | ae1dc082d7150ab47a52bbb5f72cdca877c598c3541125d18b2adc266d924613 |

Source00 is unchanged; source01 adds the first two RED tests; source02 is the
initial fix; source03 adds directory refusal to the same test; source04 is final.
All five compositions pin the same published primary and owned base above.
Each authenticates 6,520 Git blobs, 6,516 regular body/mode records and four link
targets. Every execution guards source bodies, modes and links before/after.
All final passing stages use source04. Intermediate results are retained separately.

| Owned body | Git blob | SHA256 |
| --- | --- | --- |
| Reconciler | 4e004b5042cd06fe757f45a7a3df4a277214e72f | 672c9e4af63ada9b529dae23d56d9ae8863ee251b293484878bfd52c2a1bf784 |
| Regression | 97ac730d9830fd15fb010ccdd2717ea667497741 | 3d94a5fb9cc357712918635a9b8c0ac8153ad53e455516ba832f51a18e3b1d35 |

Host Python 3.12.10:
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe,
SHA256 4d6f5f81a4bca11191c4c7c6b43632694d0a4ce74e068619d8fdc161d469859a.
Isolated tools image
sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a;
Python 3.12.3 and GCC 13.3.0. Exact g++/Python/nm/mysql_config versions and executable
hashes are retained in seal/native-build-inventory.json. Docker recovered outside
this task; no engine reset, service recovery or storage relocation was performed.

Required AGENTS/README/Plan5/finish/remaining-requirements/latest-checkpoint and
backup/audit guidance are retained. Published AI_CONTEXT.md is absent; the user
confirms the primary-local notebook is maintained and nonblocking. Refresh after
qualification preparation sees a7dbc54cadf576330529a0e126d61052161ea255: two coordination-only
documents changed, with their exact patch retained and changed sections read.
Native/schema are identical. Latest source report remains private
673ef5eb485dc6842d550b17127ffb0745464d2a2bd25b85f36e2640cced2e99: terminal
completed-present/missing-body cold continuation and corrected pure Smith capture
are source-integrated, with all execution deferred. Smith producer/compound
owner, reset cursor/unfinished prefix and special/foreign/flat routes remain open.
No private adoption or execution qualification follows from documentation.

## Commands and results

Run from the owned worktree. Exact Docker/compiler/SQL/CLI argv and terminal
statuses are retained in each stage's docker-command.json, commands.json,
launches.json, console and terminal.json. Docker uses direct D: binds and disabled
network. Initial test/fix preparation is also retained in setup/add_tests/fix
helpers; stage commands and source transports are authoritative execution inputs.

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/freeze.py 00
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/launch.py reproduce00 00
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/freeze.py 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/launch.py red01 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/freeze.py 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/launch.py units02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/launch.py native02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/launch.py cli02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/launch.py sql02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/add_directory.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/freeze.py 03
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/launch.py red03 03
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/fix_directory.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/freeze.py 04
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/finish_input_checks.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/authenticate.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/launch.py units04 04
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/launch.py sql04 04
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/launch.py native04 04
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/launch.py cli04 04
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/launch.py budget04 04
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/launch.py child04 04
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/bind_scope.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/primary_refresh.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/qualify.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/seal.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/write_docs.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-input/precommit.py
git fetch origin experimental-accounting codex/accounting-plan5
git push origin HEAD:refs/heads/codex/accounting-plan5
```

- Unchanged source: three actual FIFO CLI calls at limits 0/1/100 each hit the
  deliberately short one-second reproduction timeout; subprocess.run killed and
  reaped each child. A separate valid JSON body was grown by a fixture writer to
  33,554,433 bytes after the size observation and incorrectly returned exit 0 with
  zero findings. Four reproduction cases, no live recorded child remains.
- RED source01: the two added methods produce three growth failures and six
  TimeoutExpired errors for FIFO/symlink-to-FIFO. Focused growth uses a reduced
  bound for speed and the real main; both old Path.stat and new os.fstat seams
  author growth only after returning the original size. Source02 passes these but
  cli02 exposes directory wrapping before the regular-type check. Added directory
  cases on source03 yield three RED failures, zero errors; validation now precedes
  stream construction and a finally block closes the descriptor. Failed/intermediate
  stages are terminal and preserved; no original assertion or guard was relaxed.
- Final 16 reader modules: **425 loaded, 405 PASS, 20 opt-in skips**, no errors or
  failures. Two mapping/price budget methods plus one child-budget method then
  explicitly PASS with zero skips: **408 unique PASS, 17 remaining opt-in skips**.
  Exact skip names/reasons are in units04/terminal.json; these retain native stake,
  origins, canonical, child native and eight physical-custody integration gates.
- Independent final CLI matrix: **150 actual calls** across seven views and limits
  0/1/100. FIFO, symlink-to-FIFO, directory and /dev/zero yield **84 prompt exit-2
  refusals**, while regular/symlink-regular/hardlink-regular yield **63 unchanged
  controls**. Three additional actual 32 MiB growth calls refuse at cap plus one.
  Input bodies and native metadata remain exact except explicitly authored fixture
  growth. Another **21 in-process controls** cover successful input, all four
  nonregular types, malformed UTF-8 and malformed JSON; descriptor counts remain
  equal before/after each call. These 21 are not counted as subprocess CLI calls.
- Original mapping/price workloads: 12 near-32-MiB CLI measurements, healthy and
  damaged, limits 0/1/100; 0.651078–1.169768 seconds,
  peak 158,724,096 bytes, unchanged inputs and original
  global finding counts. Two additional oversized controls refuse. Original child
  workload: 500 roots, 32,000 children/postings, exactly 33,554,432 bytes, three
  controls; 0.840451–0.890702 seconds,
  maximum cumulative child RSS 175,504 KiB.
  Existing 30-second/256-MiB bounds pass. These component measurements do not
  establish release-host/game-loop or storage-growth budgets.
- Final native evidence: fresh original genuine restore-coin fixtures build in
  SQL and flatfile modes with unchanged providers, flags and sanitizers. Two
  executions produce five exact-identical retained claim pairs and both pass the
  independent require_integrity/ClaimProjectionFixture reader. Final native04 has
  18 g++ driver calls: 2 compile/link
  calls and 16 toolchain queries; no nested count is inferred.
  Binary/output hashes are native04/native-results.json. Intermediate native02 is
  separately retained and is not added to this final count. No maintained server
  build or service boot is claimed for this Python input change; real producers
  and complete source capture remain unqualified.
- Both unchanged disposable partial-SQL export runners PASS on
  MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1 and MySQL 8.0.46-0ubuntu0.22.04.4.
  External observation retains the first real SELECT-only snapshot and its 13
  original global findings. Each engine runs the earlier duplicate-field/control
  matrix unchanged, plus three new actual 32-MiB growth refusals: **258 added CLI
  calls total, 210 duplicate refusals, 42 original controls and six growth refusals**.
  All 47 application tables compare equal before/after on each engine; original
  capture and all resulting saved bodies remain exact. Fixture setup authors
  modeled SQL rows. This proves bounded reader/export/database immutability with
  partial modeled data, without qualifying gameplay/opening/producers.

## Evidence, notebook and open gates

Evidence D:/Dev/Tests/Duris/accounting-plan5/snapshot-input-20261008; builds
D:/Dev/Builds/Duris/accounting-plan5-snapshot-input-20261008.
qualification.json, change-scope.json, source-transport-authentication.json,
RED/green logs, actual CLI records, SQL inventories and budgets/native results
bind the above. Raw seal SHA256 bcef0ec8b940949be7c69b8794c0d79170b62e31ff0c34c1d92fd472f9230c4b covers 3,449 files,
1,781,337,239 bytes, 2,749 regular native-build files
and 14 stopped, non-OOM, network-isolated containers,
including unchanged reproduction, RED and failed intermediate runs. Native lstat
and regular-only copies/host rehash follow no link or reparse targets. Report and
follow-up are written after the raw seal and bound by the containing Git commit
and post-push delivery receipt, which rehashes every sealed file and all 27 source
overlay blobs, verifies remote equality, clean tree and preserved ancestry.

The existing shared flatfile backup fixture's seven missing provider symbols and
room fixture's UID-owner/22-function boundary remain with their primary owner;
no workaround or relaxed original runner is introduced. Seventeen opt-in tests
remain unexecuted in this slice. Complete canonical-64 fault/recovery matrices,
opening/treasury/player/routes, genuine producer/gameplay/ACK/recovery journeys,
backup/restore/retention/storage-growth release evidence, maintained combined
candidate, activation and full Plan5/R1–R8/release remain open. Bounded inventory,
synthetic fixtures and isolated passes do not complete release qualification.

No new shared interface blocker. This report and additive remote follow-up are
curator-ready notebook evidence. The primary-local notebook is nonblocking;
curator application, primary import and acknowledgment are unclaimed. No production
access/mutation, activation, audit autocorrection, deploy, merge, primary push or
cross-chat message. The goal stays active and release unqualified.

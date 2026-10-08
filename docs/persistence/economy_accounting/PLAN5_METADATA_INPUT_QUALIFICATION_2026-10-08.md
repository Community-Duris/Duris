# Plan 5 opened metadata guards and bounded reads — 2026-10-08

Backup/restore metadata reads checked path permissions, owner, links, type and
size before opening. An external writer acting after those checks could substitute
unsafe metadata or grow a valid JSON input past the existing 32 MiB cap. The old
reader accepted those inputs; a substituted FIFO blocked without a writer.

Only existing persistence_backup.read_json changes, plus standard-library errno
and io imports. It keeps secure_path's original ancestor/path checks, then opens
once with O_NOFOLLOW/O_NONBLOCK. The opened descriptor must have the existing
allowed/current owner, owner-only mode, regular-file type, one link and bounded
size. A cap-plus-one binary read enforces actual bytes on that same descriptor;
finally closes it even on validation/stream failure. An in-memory text wrapper
retains the previous default text encoding/newline interpretation and strict
JSON duplicate-key hook. No alternate-path reopen or permissive binary JSON
encoding detection is introduced. Raw bytes are released before JSON parsing.

Existing metadata_too_large, require_owner_only, unexpected_owner,
unexpected_file_type and symlink_rejected diagnostics remain fixed/redacted;
O_NOFOLLOW's ELOOP maps to the existing protected symlink diagnostic. CLI failure
remains exit 1 with empty stdout and the existing JSON error envelope. No policy,
schema, receipt field, cadence, retention or publication rule changes. No reader
repairs data or imports the independent reconciler. Fixture writers deliberately
change selected disposable copies; the reader leaves each resulting body and
metadata unchanged. Descriptor checks do not assert a quiescent filesystem or
replace the existing full-generation/digest/restore integrity checks.

## Branch, ownership and exact source

Sole local/remote codex/accounting-plan5, worktree
C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max.
Owned base f7203b4c438b35da1f1d1fcf3267d3e740a65ada; the containing commit is the result. Exact result/
remote/clean/ancestry/rehash receipt: D:/Dev/Tests/Duris/accounting-plan5/metadata-input-20261008/delivery/result.json.
All seven consolidated historical tips and prior owned slices remain ancestors.
No branch switch, force push or primary push.

Four owned files: scripts/persistence_backup.py, existing
tests/async/test_persistence_backup.py, this report and an additive
PLAN5_REMOTE_FOLLOWUP_2026-10-06.md entry. Only read_json changes; two methods are
added to the existing CapacityAndInputTests class. All prior other function/test
bodies and non-function production AST apart from the two imports stay exact.
The other 25 owned overlays, restore implementation, review-remediation tests
and original integration/native recipes remain exact. Source00-to02 changes only
the two code/test paths. change-scope.json binds these claims.

Shared interface/schema requests: none. Shared coordinator/contracts/producers,
registry/matrix/migrations/activation remain primary-owned. Preserve all 27 owned
overlay prerequisites during integration. Inactive behavior, wallet-root
ITEM_MONEY exclusions and the declined inactive spell path remain intact.

Tested primary a7dbc54cadf576330529a0e126d61052161ea255; fixed primary plus 27 owned overlays
3a7fe631eefeb0d5864b3e3419f72dac11e0e31e, archive SHA256 a75ded30ec6f3397d40f34336226a8af32abb10e72bd2a1c2134ff5096039ee7.
Native tree 833d3085815b396861ad18a77635412212381e4b; migrations tree 7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2, canonical
head 64. Historical owned native source is not the native execution base.

| Transport | Composed Git tree | Archive SHA256 |
| --- | --- | --- |
| 00 | 280c829f47812f86f155cb4e584b17ffe3355d41 | d620e8a05d8c1c7854618c0137b11e74d32f772ab1fca630cf83a7d8997c8277 |
| 01 | 4880fcc70bd85826bc3474d0677dce05ab50cb9a | 45008614098d0c29c6b6097efc5824be7c123df31770b859cef1e301978e4b10 |
| 02 | 3a7fe631eefeb0d5864b3e3419f72dac11e0e31e | a75ded30ec6f3397d40f34336226a8af32abb10e72bd2a1c2134ff5096039ee7 |

Source00 is unchanged, source01 adds the RED tests, source02 fixes the reader.
All three pin the same primary/base. Each transport authenticates 6,520 Git blobs,
6,516 regular body/mode records and four link targets. Every stage guards source
bodies/modes/links before/after; all passing qualification stages use source02.

| Owned body | Git blob | SHA256 |
| --- | --- | --- |
| Backup reader | 0cc290bb836b78e4dc8c826daec67b6f50bfdaba | 1e22f6c46c7f065b5679bf03c6c15cf1219293eb4444039cef59c4efbb6c7f92 |
| Regression | e45428aca514f9b9eccdcfddda0c3f470ae55af1 | 11c121a447686b5cb114bb20b73494da0752995ccbdfd6b407871761cd8ea74f |

Required AGENTS/README/finish/remaining-requirements/Plan5/checkpoint and backup/
audit guidance are captured. Published AI_CONTEXT.md is absent; the user confirms
the primary-local notebook is maintained and nonblocking. Latest refresh
a7dbc54cadf576330529a0e126d61052161ea255 equals tested primary. Its latest checkpoint reports
private 673ef5eb485dc6842d550b17127ffb0745464d2a2bd25b85f36e2640cced2e99
terminal cold recovery/corrected Smith capture source composition; implementation
and major-plan execution remain private/deferred/unqualified. This slice does
not qualify that candidate, all 27 overlays or the combined release.

## Commands, runtimes and measured results

Host Python 3.12.10 at
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe,
SHA256 4d6f5f81a4bca11191c4c7c6b43632694d0a4ce74e068619d8fdc161d469859a.
Isolated image
sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a,
Python 3.12.3/GCC 13.3.0. Exact tool versions/hashes are in the native seal.
D: task roots are mounted directly, with network disabled and separate bin
outputs. No Docker recovery/reset/storage relocation occurred in this task.

Commands run from the owned worktree. Exact original compiler/SQL/native/CLI argv,
return statuses and timing are retained in stage docker-command.json,
commands.json, launches.json, logs and terminal.json; preparation scripts are
retained. No original guard/recipe/assertion is bypassed or relaxed.

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/freeze.py 00
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/launch.py reproduce00 00
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/add_tests.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/freeze.py 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/launch.py red01 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/fix.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/freeze.py 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/authenticate.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/launch.py units02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/fix_cli_policy.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/launch.py cli02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/setup_disposable.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/launch.py disposable02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/add_input_checks.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/launch.py inputs02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/bind_scope.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/primary_refresh.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/qualify.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/seal.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/write_docs.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/write_docs_fixed.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-metadata-input/precommit.py
git fetch origin experimental-accounting codex/accounting-plan5
git push origin HEAD:refs/heads/codex/accounting-plan5
```

- Unchanged reproduction: seven real filesystem cases. Oversize growth, weakened
  permissions, extra hardlink, substituted symlink and changed UID are falsely
  accepted (five). The FIFO case hits the deliberate two-second timeout and is
  killed/reaped; directory already refuses with the generic OSError envelope.
  All changes are authored after the original security checks return. The growth
  seam returns the original size before appending valid JSON whitespace to
  33,554,433 bytes. These are actual filesystem changes, not mocked auditor results.
- RED source01: two added methods, **six failures and one TimeoutExpired error**,
  zero skips. The growth case uses the actual 32 MiB cap. Guard cases invoke an
  actual child reader; the root-only foreign-UID case executes on this root-owned
  isolated image. On a non-root runner that one subcase is inapplicable; the
  remaining cases still run. Final unchanged filesystem/review modules:
  **68 PASS, zero failures/errors/skips**.
- Actual CLI matrix: **60 subprocess calls** over both configured backend modes.
  Four metadata consumers (policy, generation manifest, completion status and
  schedule) times seven race classes times two modes yield **56 exit-1 refusals**
  with exact existing redacted codes, plus **four unchanged standalone controls**.
  A disposable wrapper injects only the external writer seam around actual main;
  controls invoke the original script directly. Original generation/live/journal
  inventories stay exact, no staging appears and each reader leaves the fixture's
  resulting input unchanged. These generations model authority and are not SQL
  producer/gameplay evidence.
- Eight additional direct-reader controls accept exactly 33,554,432 bytes and
  Unicode, refuse cap-plus-one, duplicate/escaped-duplicate keys, malformed UTF-8,
  malformed JSON and empty input, preserving bodies and descriptor counts.
- **Three original native integration methods PASS, zero skips**: complete
  MariaDB and MySQL dump/schema/history/value/isolated-boot methods, plus the
  original flatfile pending-journal/receipt replay, account/player/domain load and
  boot journey. Both SQL super-method bodies and the inherited flatfile method
  remain exact. All three real service observations reach Entering game loop.
  Fresh original fixture builds use unchanged providers/flags/sanitizers:
  **20 g++ driver calls, four compile/link calls and 16 toolchain queries**.
  No nested compiler count is inferred. Genuine SQL/flat claim fixtures are
  compiled and consumed by the original integration; no substitute provider or
  audit result is supplied.
- SQL extension: original captured generations are imported into two fresh,
  network-disabled sources on MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1 and
  MySQL 8.0.46-0ubuntu0.22.04.4. Six metadata targets (policy, manifest, completion,
  schedule, drill, independent tombstones) times seven races times two engines
  give **84 observed refusals**, before new capture or restore candidate creation.
  These include 28 in-process main calls and 56 direct consumer calls, not 84 CLI
  subprocesses. Original generation and selected SQL claim rows remain exact;
  every resulting fixture input and descriptor count stays unchanged. Two normal
  controls then complete real new SQL captures and report healthy. Original
  qualification records contain **50 calls, 22 expected fault refusals**.
  Fixture seeding is explicit; selected-claim immutability does not imply a full
  inventory of every SQL table or real player/producer journeys.

Maintained servers are reused only after authenticating every repository/image
dependency against the tested archive and current tools image. They were
historically built with 754 translation units per mode; no fresh maintained
server build is claimed for this Python-only change.

| Reused server | SHA256 | Repository dependencies | Image dependencies |
| --- | --- | --- | --- |
| flatfile | 9462829d443f5e138feac60c30bb27c8ec81b921fadfb2862be4a5386e3b54be | 1320 | 20 |
| sql | 1a7e305e8be3f516ae6778d6eb03fadcb14859e0876df6d279fc1d4bc75d937b | 1318 | 28 |

## Evidence, notebook and remaining gates

Evidence D:/Dev/Tests/Duris/accounting-plan5/metadata-input-20261008; task builds
D:/Dev/Builds/Duris/accounting-plan5-metadata-input-20261008.
qualification.json, source transport/scope, RED/green logs, CLI/input results,
metadata observations, original test events/qualifications/service logs and reuse
bindings authenticate the claims. Raw seal SHA256 ade3527dd06ba8d6ea760ec8ff4c57629d1c0c3ae1e7a7d800217506e4102f88 covers
1,896 files/1,973,967,379 bytes, 14
regular build files and 7 stopped, non-OOM, network-isolated
containers including unchanged/RED runs. Native lstat/regular-only retention and
host rehash follow no links/reparse targets; physical SQL datadir/private keys are
not copied into selected temporary evidence. Reports/follow-up written after the
raw seal are bound by Git and post-push delivery. Delivery rehashes every sealed
file, all 27 overlay blobs, verifies remote/clean state and preserved ancestry.

The complete original 12-case backup integration module is not claimed: its
established missing-provider economic flatfile fixture boundary remains with the
shared owner. The independent room UID-owner boundary also remains. No full
canonical-64 fault matrix, all-route writer/producer/gameplay/ACK/opening coverage,
release-host latency/storage-growth/retention budgets or private combined-candidate
qualification follows. Full Plan5/R1–R8/release/activation remain open; synthetic
fixtures and isolated passing tests cannot close them.

The first post-seal report-authoring attempt referenced latest-primary/result.json
instead of the existing primary-refresh/result.json and failed before writing any
repository file. Its original helper stays sealed unchanged; corrected
write_docs_fixed.py and a separate authoring record are retained outside the raw
seal. No product/test rerun or evidence replacement was needed.

No new shared interface blocker. This report and additive remote follow-up are
curator-ready input; the primary-local notebook remains nonblocking. Curator
application, primary import and acknowledgment are unclaimed. No production data,
activation, audit autocorrection, deploy, merge, primary push or cross-chat message.
The full goal stays active and release unqualified.

# Plan 5 saved-snapshot JSON ambiguity — 2026-10-08

The independent saved-snapshot CLI previously accepted repeated JSON object keys.
Python silently selected the last value before any reconciliation. An earlier
conflicting balance, completion flag, root result, source event or original plan
could disappear, yielding zero findings and exit 0. Equal-value duplicates and
escaped spellings of the same decoded key were also accepted. A private/unknown
field's duplicate was ignored rather than treated as ambiguous input.

The existing main parser now supplies a local object_pairs_hook which rejects any
repeated decoded field with SnapshotError("duplicate snapshot field"). This runs
at every object depth before constructing the auditor or selecting a view. Exit 2,
empty stdout and fixed stderr `reconciliation failed: duplicate snapshot field`
preserve the existing malformed-input contract without echoing names, aliases,
values or paths from the input. Unambiguous snapshots keep their existing values,
views, limits, findings and exit statuses. No mutation codec or backup parser is
imported, no data is repaired, and no schema/API/field contract changes.

## Branch and scope

Sole local/remote codex/accounting-plan5; worktree
C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max.
Owned base 6b5d21a8a21f82f951a6df9ae03c9d3911c8ee37. The containing commit is the result; exact result/
remote/clean/ancestry/rehash evidence is D:/Dev/Tests/Duris/accounting-plan5/snapshot-json-20261008/delivery/result.json.
All seven consolidated historical tips and prior owned fixes remain ancestors.
No branch switch or push to experimental-accounting.

Four owned files: scripts/reconcile_economy_accounting.py, the existing
tests/async/test_reconcile_economy_accounting.py, this report and additive
PLAN5_REMOTE_FOLLOWUP_2026-10-06.md. Only existing main changes; one method is added
to the existing test class. Every prior other function/method and the other 25
owned overlays remain byte-exact. change-scope.json binds AST/body scope and the
composition delta. Shared coordinator/contracts/producers/migrations/native
recipes/registry/matrix/activation are untouched. Inactive behavior, wallet-root
ITEM_MONEY exclusions and the declined inactive spell path remain. Existing
27-overlay prerequisites must be preserved during import.

Shared interface/schema requests: none. Saved JSON now refuses ambiguous duplicate
fields rather than selecting a competing value. The existing integer/identity/
plan/policy interpretation and the independent mutation-free audit stay intact.

## Exact tested source

| Input | Identity |
| --- | --- |
| Tested published primary | 6d223ebe1a494326c2613e9371b47e50c41f0951 |
| Fixed primary plus 27 owned overlays | 0a2bd861467d34f6ff6666c5a012bad4aa66b5dd |
| Fixed archive SHA256 | 4fd30a74d3b95891b6cd732fce3412c88edc0d877622d2eeb7c6c9d105112bc8 |
| Published native tree | 833d3085815b396861ad18a77635412212381e4b |
| Migration tree, canonical head 64 | 7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2 |
| Reconciler blob | db47bf0b6f6c1a0b83a75b376457a5f1056ff020 |
| Reconciler SHA256 | 5943aad5aea870ff0d10002e91cc397a67d537c9a65c73bf4a8e769792923012 |
| Test blob | cef6cfc62653b3737398fd49f9469dc1de58f9fc |
| Test SHA256 | 0c55ba3cf5ebeac9a47d675100529fa033abb147999513b22892344a78b86624 |
| Tools image | sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a |

Unchanged source00 a33c68d1e17dda9ef1bae771e4e67eba2284f874 and RED
source01 df646a5fdb637548fd5bf97b298f686c2d2a5195 use primary
138c379e42940cdfdc0d077054b35b76f5bd2016. Primary tracking advanced during work;
fixed source02 uses the table's newer primary. The only additional primary changes
are two retained coordination documents, read in full; native/schema are exact.
The source00-to02 delta contains those docs and exactly the two owned code/test
changes. All passing stages use source02. Each transport authenticates 6519 Git
blobs, 6515 regular body/mode records and four link targets. Every stage guards
source bodies, modes and links before/after. Historical local native source is not
the native test base; this bounded change does not qualify all included overlays.

Raw AGENTS/README/Plan5/finish plan/remaining requirements/latest checkpoint/backup
and audit guidance are retained. Published AI_CONTEXT.md is absent; user confirms
the primary-local notebook is nonblocking. Latest primary ab67bad7e079245128c4de04a82beed0df2a2e9e
is docs-only with exact native/schema. Its new source report records private
673ef5eb485dc6842d550b17127ffb0745464d2a2bd25b85f36e2640cced2e99: completed-present/
full-missing-body terminal cold recovery and corrected pure Smith capture are
source-joined, 204 paths/165 source. All execution remains deferred. Smith executable
ownership/registration, reset cursor/unfinished-prefix and special routes remain.
Original valid constructors have no normal returned-null branch; the invalid-Rnum
guard is separate from accounting refusal. No invented constructor-failure phase
or historical global construction chronology is added. Native/schema/public source
are unchanged; no private import/adoption/execution qualification is inferred.

## Commands, results and limits

Host Python3.12.10, executable
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe,
SHA256 4d6f5f81a4bca11191c4c7c6b43632694d0a4ce74e068619d8fdc161d469859a.
Commands run from the owned worktree. Exact Docker/compiler/SQL/CLI argv is retained
in stage docker-command.json, commands.json and launches.json; added SQL CLI
records are also in sql03/{mariadb,mysql}/commands.json. Network is disabled and
D: task outputs are mounted directly.

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/reproduce_host.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/freeze.py 00
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/freeze.py 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/launch.py red01 01
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/freeze.py 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/authenticate.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/launch.py units02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/launch.py native02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/launch.py sql02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/launch.py budget02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/launch.py sql03 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/launch.py child02 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/launch.py child03 02
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/bind_scope.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/primary_refresh.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/qualify.py
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -X utf8 D:/Dev/Temp/accounting-plan5-snapshot-json/seal.py
```

- Unchanged host reproduction: seven actual CLI inputs report zero exceptions and
  exit 0 despite duplicated fields. Raw bodies, commands, stdout/stderr and hashes
  remain in host-reproduction01. RED authenticated source01: new method has
  **168 failing subtests, no errors/skips**, across eight damaged inputs, all seven
  views and limits 0/1/100. The 21 unambiguous controls already pass. Equal-value,
  Unicode-escaped and private-name duplicates are included.
- Full 16 reader modules: **423 loaded, 403 PASS, 20 opt-in skips**, no errors or
  failures. The two mapping/price budget skips and the one child-budget skip are
  then explicitly executed: **three more PASS, zero skips**. Unique tested set is
  **406 PASS, 17 still-unexecuted opt-in tests**. Exact skip names/reasons are in
  units02/terminal.json. They cover native stake/origins/canonical/child and eight
  physical custody integrations. No skip is claimed as a pass.
- Parser performance: the unchanged mapping and price budget methods run 12 CLI
  measurements over nearly 32MiB input, healthy and damaged, at limits 0/1/100.
  They observe 0.647730–1.355067 seconds,
  peak 158,564,352 bytes, with original global finding
  counts, bounded output and unchanged inputs. The mapping method additionally
  refuses two oversized inputs. The original child workload runs 500 roots,
  32,000 children and 32,000 postings at exactly 33,554,432 bytes; three CLI controls
  pass at 0.761385–0.975828 seconds,
  maximum cumulative child RSS 189,116KiB.
  All existing 30-second/256MiB bounds pass. These are synthetic component
  workloads on this image, not release-host/game-loop/storage-growth budgets.
- Native checks: original genuine restore-coin fixture compiles separately for SQL
  and flatfile, with unchanged source/provider/flags and sanitizers. Two executions
  produce exact-identical five retained claim capsule pairs. The independent
  require_integrity/ClaimProjectionFixture reader accepts both. Binary/output hashes
  are native02/native-results.json. GCC driver calls: 18, comprising
  2 fresh compile/link calls and 16
  toolchain queries. No nested compiler count is inferred. No maintained server
  build or service boot is claimed for this parser change. Native capsule acceptance
  does not qualify real claim producers or complete source capture.
- Disposable SQL: both original partial-export runners execute unchanged and PASS
  on MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and MySQL8.0.46-0ubuntu0.22.04.4.
  An external observation wrapper retains the first real SELECT-only capture and
  leaves its 13 existing global findings intact. Five duplicate-field classes are
  authored only in saved copies and tested across all views/limits: **210 exit 2
  refusals plus 42 original controls, 252 added actual CLI calls**. All 47 application
  tables compare equal before/after on each engine, as do the original captured
  snapshot and saved bodies. Source setup authors modeled SQL fixture rows; this
  is genuine database/export/permission/immutability evidence with partial modeled
  accounting data, not real gameplay/opening/producer coverage.
- Failed evidence-authoring attempts are preserved: setup initially used an invalid
  equal-dict assertion for a newly added private property; sql02 was refused by
  the unchanged runner's required disposable socket-prefix guard; child02 lacked
  its original required artifact-directory environment. Corrected observation
  helpers use fresh roots/stages. Product source remains source02 throughout SQL/
  child retries; no original test body/guard/assertion is relaxed or bypassed.

## Evidence, notebook and release gates

Evidence D:/Dev/Tests/Duris/accounting-plan5/snapshot-json-20261008; builds
D:/Dev/Builds/Duris/accounting-plan5-snapshot-json-20261008.
qualification.json, source transport/scope, host reproduction, RED/green logs,
SQL table inventories/CLI records and budget/native results bind these claims.
Raw seal SHA256 ef28ccc9926b74c569a13933dad34e135951af4544ae030629689548ac30e211 covers 1764 files/990,550,924 bytes,
1380 native build regular files and 9
stopped, non-OOM, network-isolated containers, including retained RED/failed
adapter stages. Native lstat/regular-only retention and host rehash do not follow
links/reparse targets. Delivery verifies every sealed file, all 27 overlay blobs,
remote equality, clean worktree and preserved ancestry after push.

Remaining opt-in qualifications are explicitly unexecuted in this slice. The
established original flatfile backup/provider-link and room UID-owner boundaries
remain with their shared owner. No full canonical 64 fault matrix or maintained
server build rerun is claimed. Complete opening/treasury/player/routes, real
producer/gameplay, fault/load/ACK/recovery, retention/storage-growth budgets,
private combined candidate, activation and full Plan5/R1–R8/release remain open.
No new publication blocker. No production data, accounting activation, audit
correction, promotion/deploy/merge, primary push or cross-chat messaging.

This report and additive remote follow-up are curator-ready notebook input. The
primary-local notebook remains nonblocking; curator application, primary import
and acknowledgment are unclaimed. The goal remains active and release unqualified.

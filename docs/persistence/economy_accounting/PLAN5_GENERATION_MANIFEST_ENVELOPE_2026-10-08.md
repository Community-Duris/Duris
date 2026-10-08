# Plan 5 generation-manifest envelope — 2026-10-08

Generation verification accepted JSON `true` and `1.0` as version 1. Non-object
manifests reached `.get()` and raised uncontrolled AttributeError, which escaped
the CLI's expected refusal handling. An unchanged-reader filesystem reproduction
establishes both defects in both backup modes. The existing verifier now requires
a JSON object and exact integer version 1 before reading the remaining fields.
All malformed object/version cases produce `invalid_generation_manifest`.
Canonical version 1, checksum/schema, ownership and retention checks remain.

## Branch, owned scope and prior-work dependency

Sole local/remote `codex/accounting-plan5`; worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `0ab16c81cab39c318acc880ca71d1e75fc075731`. The containing commit is this issue's result;
post-push `delivery/result.json` records its exact SHA and verified remote.
All seven previously consolidated tips remain ancestors. No branch switch,
history rewrite or primary push occurs.

Owned changes are `scripts/persistence_backup.py`, the existing
`tests/async/test_persistence_backup.py`, this report and additive
`PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`. Only production function `verify` changes;
two existing-file test methods are added and every prior method remains exact.
`change-scope.json` records the AST/body bindings. No new validation helper,
dependency, format, schema or shared interface is introduced.

The original generation format already requires version 1, backup mode,
generation identity, integer creation time and checksum/schema fields. This fix
makes the object/version representation explicit. It adds no storage field,
interprets no missing metadata and mutates no captured authority. Existing callers
are generation discovery, backup/rotation, status/finalization and restore.
The new regression consumers check direct verification plus restore and CLI
refusal before candidate/service/private-DB/native-qualifier work.

The primary still lacks earlier owned tombstone fix
`c373f04fac0ef14f734bc086c1cef01246dc70fe`. The tested 27-overlay composition
explicitly includes its unchanged restore blob
`23b17a3d158ac455105de1cd1303561dcc028ad8` and original regression alongside this
new fix. Primary integration should preserve/import that commit as well to
reproduce this candidate. This follows up prior work on the expected remote
branch; it does not edit the tombstone implementation again. Earlier 24-overlay
results retain their recorded scope and are not relabeled as covering that blob.

Shared coordinator/contracts/producers/registry/matrix/activation, migrations
and original native recipes/harnesses are untouched. Inactive behavior,
wallet-root ITEM_MONEY exclusions and the declined inactive spell change remain.
No audit finding is corrected and no production operation occurs.

## Exact tested source and commands

| Input | Identity |
| --- | --- |
| Refreshed primary | `315bf5ac7a5e21af061b8e1d643a50e9cdb74a47` |
| Fixed primary plus 27 owned overlays | `8b6b40d9caeec860d0cc5cce576529f3183bd417` |
| Fixed archive SHA256 | `5984b45083f96fbf8dae3786a995f2c032ee03d57403b289276892304f4dd15e` |
| Native tree | `833d3085815b396861ad18a77635412212381e4b` |
| Migration tree | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Canonical schema head | 64 / 0064_auction_custody_history |
| Tools image | sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a |

Reproduction source00 tree962daa232bd52c47899fa3dc2c68fc77a44e7720 contains the
unchanged verifier. RED source01 tree2db44b23e2b378e65b1e0bf3a556b9bac3980f7b adds
only the two regressions. Fixed source02 is used by both complete filesystem
modules and the entire existing 12-case native backup module. All three archives
authenticate 6,512 Git blobs, 6,508 regular bodies/modes and four link targets.
Before/after guards check every source body, mode and link. Local shared `src`
is older 4abb609524a1f1682ea4c190f82d75003c4d679b and is not the native test base.
AGENTS/README/Plan5/finish plan/requirements/checkpoint are retained as raw bytes.
Published AI_CONTEXT.md is absent; the user-confirmed primary-local notebook is
nonblocking.

Commands ran from the owned worktree with D: temp/cache/build/evidence roots:

```text
python -X utf8 D:/Dev/Temp/accounting-plan5-generation-envelope/freeze.py 00
python -X utf8 D:/Dev/Temp/accounting-plan5-generation-envelope/launch.py reproduce00 00
python -X utf8 D:/Dev/Temp/accounting-plan5-generation-envelope/freeze.py 01
python -X utf8 D:/Dev/Temp/accounting-plan5-generation-envelope/launch.py red01 01
python -X utf8 D:/Dev/Temp/accounting-plan5-generation-envelope/freeze.py 02
python -X utf8 D:/Dev/Temp/accounting-plan5-generation-envelope/authenticate.py
python -X utf8 D:/Dev/Temp/accounting-plan5-generation-envelope/bind_reuse.py
python -X utf8 D:/Dev/Temp/accounting-plan5-generation-envelope/bind_original_methods.py
python -X utf8 D:/Dev/Temp/accounting-plan5-generation-envelope/launch.py units02 02
python -X utf8 D:/Dev/Temp/accounting-plan5-generation-envelope/launch.py cli02 02
python -X utf8 D:/Dev/Temp/accounting-plan5-generation-envelope/launch.py whole02 02
python -X utf8 D:/Dev/Temp/accounting-plan5-generation-envelope/qualify.py
python -X utf8 D:/Dev/Temp/accounting-plan5-generation-envelope/seal.py
```

Linux Python3.12.3/GCC13.3/nm2.42/mysql_config10.11.14 and executable hashes are
retained in the seal inventory. Containers have no network/ports, private RAM
source/tmp, direct D: task build binds, 2 CPUs/5GiB. Original integration flags,
fixtures, assertions and compiler argv stay intact. Native lstat precedes
regular-only evidence copies; database physical files, backup archive bodies,
private keys and service runtime trees are excluded. Existing services and
volumes are untouched. The full native stage uses its original SYS_ADMIN/
seccomp settings for isolated restore-capacity checks.

## Results, limitations and shared handoff

| Check | Result |
| --- | --- |
| Unchanged-reader reproduction | 16 cases; 4 accepted aliases and 12 uncontrolled AttributeErrors |
| RED, two new methods | 20 failures/32 errors, terminal exit 1 preserved |
| Complete backup filesystem module | 43 PASS, zero skips |
| Complete backup-review remediation module | 17 PASS, zero skips |
| Actual CLI, real policy parser/dedicated restore mount | 34 controlled refusals, zero candidates/source changes |
| Fixed filesystem stage | 17.752827s, zero live children |
| Full original native backup module | 12 executed, 11 PASS, one original fixture-link failure, zero errors/skips |
| Native stage | 645.455535s; 726 commands/748 launches; terminal exit 1 preserved |
| Actual isolated service boots | Ten: 8 flatfile, 1 MariaDB, 1 MySQL |
| Database qualification per engine | 23 calls: 12 accepted/11 expected refusals |

New tests exercise 17 malformed manifests per mode: non-objects, missing version,
booleans/floats/strings/null/containers and unsupported integer versions.
There are 34 direct verify refusals, 34 direct restore refusals and 34 CLI restore
refusals. Positive canonical manifests are reverified. Generation, live authority,
journals and independent erasure ledger remain unchanged; no candidate, service,
private database or native qualifier is reached. These 60 tests use disposable
filesystem fixtures and modeled SQL dump bytes, not real SQL capture. The RED
errors preserve uncontrolled .get() failures and aliases advancing into mocked
restore; residual candidates also cause later subtest failures. They are not
counted as additional distinct defects. GREEN has no failures/errors/skips. A separate 34-case actual subprocess CLI check
uses the real policy parser, source security checks and the original 512MiB
dedicated tmpfs recipe. Every invocation returns only the fixed JSON refusal
without traceback/private alias and leaves sources/ledger/policy unchanged.
No native binary is supplied or reached; dump bytes remain modeled. See
cli02/cli-results.json and exact commands/terminal records.

The complete native module supplies separate compatibility evidence for genuine
current64 captures, full dump/import/schema/value checks on actual
MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and MySQL8.0.46-0ubuntu0.22.04.4, WAL
recovery/quarantine, pending replay, bank/domain exactly-once recovery, lifecycle
retention, locker/spell receipts, foreign-owned boot and corrupt lazy-catalog
refusal. Both reused 754-unit maintained servers are body/dependency authenticated:
SQL SHA1a7e305e8be3f516ae6778d6eb03fadcb14859e0876df6d279fc1d4bc75d937b
with 1318 repository/28 image inputs; flatfile
SHA9462829d443f5e138feac60c30bb27c8ec81b921fadfb2862be4a5386e3b54be
with 1320/20. No fresh maintained make/build is claimed for this Python change.

The integration file is unchanged from the owned base, blob
d1ef31c19e79228e994e5c764a8553875db0f86e. Its nine
prior Plan5 method/helper differences from primary remain explicit; nine critical
shared native recipes/fixtures equal primary. The inherited observer's textual
plus24 label is superseded by authenticated source02's 27-overlay inventory.

The sole failed test remains
`test_flatfile_economic_record_loss_refuses_before_service_boot`. Its unchanged
strict ASan/UBSan/no-GC authority fixture fails linking the same seven symbols:
auction_native_command_decode, lockpick_retirement_payload_valid,
native_quest_cost_projection_decode/encode and
native_quest_coin_give_decode/encode/project. Its nine missing-file controls are
unreached. Genuine provider closure is primary-owned; exact compiler command,
symbols and provider body pins are retained in qualification.json and the prior
PLAN5_CURRENT64_FULL_BACKUP_MODULE_HANDOFF_2026-10-08.md. No stub, fake UID owner,
removed branch, relaxed flag or shared recipe edit is supplied. After primary
closure, rerun the original fixture/all nine controls and complete 12-case module
on the combined source. Full native module qualification remains false.

## Evidence, notebook and remaining gates

Evidence D:/Dev/Tests/Duris/accounting-plan5/generation-envelope-20261008;
helpers D:/Dev/Temp/accounting-plan5-generation-envelope; builds
D:/Dev/Builds/Duris/accounting-plan5-generation-envelope-20261008.
Start with source00/01/02.json, source-transport-authentication.json,
reproduce00/unchanged-reader-reproduction.json, red01/unit logs, change-scope.json,
original-method-bindings.json, qualification.json, each terminal/commands record,
seal/evidence.json and post-push delivery/result.json.

Raw seal SHA256 e6da9fb7611db292cac54954c0682be22cd7646eed7a24e79ab17ada699634fb covers 1,934 files / 2,098,405,301B,
20 native build files, zero copied links/reparse points.
All six containers terminate without OOM; reproduction/unit/CLI/inventory exit 0,
RED and native module exit 1 are retained. No live recorded child remains.
The post-push receipt rehashes every sealed body and 27 overlays and verifies the
remote result and seven preserved ancestor tips.

This report/follow-up is additive curator-ready notebook input. Application or
acknowledgement on the primary-local system is not claimed or a blocker.
Genuine original authority/room-seed UID-owner/SQL-baseline recipe closure,
private combined native implementation, producers/source installation, full
opening, player/route/fault/load journeys, Plan5/R1–R8 and release remain open.
At terminal refresh, primary abc763fc197fae4aa4609d21c00ae78d191d9bd2 changes documentation only;
tested native/migration inputs remain exact. No private implementation is qualified.
No activation, production access/write, autocorrection, deployment, merge or
experimental-accounting push occurs. This fixes the owned manifest preflight;
it does not certify the full accounting release.

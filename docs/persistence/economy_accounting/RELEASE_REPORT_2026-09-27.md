# Economy accounting release qualification — 2026-09-27

**Decision: BLOCKED.** This is an audit progress record, not an activation or
deployment authorization. The release validator refuses the current writer
inventory, and neither backend has a certified native-authority audit export or
the complete player and fault journey evidence required by Plan 5.

## Harvest audit checkpoint

Commit `edde723168b40cbc0d51380fc7a2cd381675b3cf` on
`add-double-entry` includes the kingdom harvest source review and the latest
merged auction claim code. The registry has **562 route rows** and 2,768
lexical occurrences (2,710 unique path/line/family sites). It maps 1,584
unique sites; **1,126 remain unmapped**. The generated matrix check and its
route-count contract pass at this commit. The validator passes contract
validity and the release validator still refuses missing executable writer
evidence.

All 11 previously unmapped sites in `src/kingdom/kingdom_harvest.c` now have
source classifications: provisional node/material allocation and rejected
material cleanup; world-node spawn, reap, periodic retirement and shutdown
unload; realm resource harvest; and personal material grants with node
retirement. The current source refuses these legacy effects during an active
accounting epoch. It does not yet prove one root that binds the node charge,
realm store or material output, actual item owner, and any terminal node
retirement. `obj_to_room` can reroute or destroy the spawned node, while the
personal gather path counts an item after a void `obj_to_obj` call; those
actual outcomes need exact retained evidence.

The full 48-test writer source-contract suite passed on the pre-auction-merge
audit commit `67bbd190ce802cfcdaba7ca8a613ad3c695d799c`. After the auction
merge, the harvest and typed-submit contracts, validator, generated matrix
check and route-count contract passed on the integrated source; the matrix's
auction function line anchor was refreshed before the push. These checks are
source and synthetic contracts, not real player or backend release journeys.

The subsequent logical item-source update through `65f72ad4c` changed six
typed-submit declaration excerpts and shifted related source lines without
adding or removing lexical calls. The registry sites and matrix definition
lines were reanchored. On that merged source tree, all 48 writer contracts,
the accounting validator and the generated-matrix check passed; coverage
counts remained 562 routes, 1,584 mapped and 1,126 unmapped unique sites.

## Earlier clean integrated audit checkpoint

The tested audit snapshot is commit
`a7dd9ed7dabf8a661236e607372a76768e5fa233`, which includes merged
`add-double-entry` work through `2d562a128` and the salvage census
review. This is an audit checkpoint, not a release build. The registry has 553
route rows and 2,768 lexical occurrences (2,710 unique path/line/family
sites). It maps 1,573 current unique sites; **1,137 remain unmapped**. The
matrix still records `coverage_complete=false` and
`playable_release_status=BLOCKED`.

The salvage review linked all 14 previously unmapped lexical sites in
`src/item/salvage.c`: eight unpublished output allocations, one rejected
candidate cleanup, and five live source-item detach/destruction sites. It
also records the optional `vnum_from_inv` scientific-tools consumption that
the lexical scanner does not detect. Output grants are queued separately
from source and tool retirement. The command has a source-order active-epoch
refusal, but no same-root salvage operation or executable backend journey is
qualified. The replayed-source invariant fixture now asserts the current
global source-event rejection message and passes.

| Check on the audit snapshot | Result |
| --- | --- |
| `python scripts/validate_economy_accounting.py` | Passed: 13 fixtures, 553 writer routes, 2,768 candidate occurrences. Contract validity only. |
| `python scripts/generate_economy_writer_coverage.py --check` | Passed: 553 matrix rows, 1,573 mapped and 1,137 unmapped unique sites. |
| `python tests/async/test_economy_writer_coverage_contract.py` | All 47 source-contract tests passed after the merge and matrix refresh. |
| `python tests/async/test_reconcile_economy_accounting.py` | All 16 synthetic read-only reconciliation fixtures passed. |
| `python tests/async/test_audit_accounting_invariants.py` | All 9 invariant fixtures passed. |
| Salvage-specific `test_pa_item_admission.py` method | Passed the source-order refusal check. The full 26-test file reported 22 passes and four environment errors in the isolated Windows checkout: two methods require a local compiler and two require generated `areas/world.obj`. |
| `python scripts/validate_economy_accounting.py --release` | Refused: `writer has no executable evidence`. |

No new C/C++ code was written in this audit checkpoint. The SQL and flatfile
builds and disposable database runs below used earlier source snapshots; they
do not certify this integrated commit. No real player journey, native-authority
export, full backend replay, or release workload was run on this checkpoint.

The later branch checkpoint `6bcd83830dabf4d4725ce6404dde2fe031d8014e`
merged the world quest source-ID update and reanchored seven census entries
in `src/world/world_quest.c` by ten lines. The lexical occurrence and mapped
counts above did not change. The validator, generated-matrix check, focused
typed-submit and salvage contracts, 16 reconciler fixtures and 9 invariant
fixtures passed after that merge; the full 47-test writer suite belongs to
the preceding `a7dd9ed7d` audit snapshot.

At `e8ad2228ed0ecbca8b6cbe3cda7fcaef94140257`, the immutable migration
verifiers `0036` through `0040` were marked executable in Git. A Git archive
of that commit lists all five scripts with executable mode, without content
or checksum changes. The lifecycle manifest validator passed with 220
database tables and 34 non-database stores. The Windows migration-runner
suite reported five platform errors involving symlink privilege, Unix UID
checks and Bash paths; its canonical verifier method passed but does not
establish Linux execution. Fresh/upgrade/replay database qualification and
the earlier runtime metadata mismatch remain unverified on this checkpoint.

## Source and test identity from the earlier shared-tree investigation

The initial Git base was `49af585c4b9c8cfa5ead0ac07025f39d4720a659`
on `add-double-entry`; the shared branch advanced during qualification and
was at `631c2cfdf` before an earlier full contract check. Focused Python
checks and both server builds ran against a **moving,
uncommitted working tree** with concurrent changes from other persistence
tasks. There is no single integrated commit to certify from those runs. The
disposable SQL runs used a clean `git archive` of the initial `49af585c4`
commit, not the later uncommitted tree. The release gate must be rerun on one
final integrated commit after Plans 1–4 land.

The clean `cfd852ea5` merge checkout has a different source census from the
shared working tree used for the matrix below. Its scan found 2,763 lexical
occurrences and 2,705 unique sites: 1,418 registry sites still present, 1,287
unmapped current sites, and 129 registered sites absent at their recorded
path/line/family. The stored census has 280 removed and 284 added exact
entries against that clean checkout. Some concurrent source changes are still
uncommitted in the shared tree; regenerate and recheck the census on the final
integrated commit before treating the matrix as current release evidence.

## Runtime contract qualification on `8cf1fb544`

The 0040 global activation table is now in the 220-table runtime inventory,
and the migration head and both engine fingerprints were measured and pinned.
The C++ boot query and shell verifier both include the activation receipt and
global decision in detailed column-type and CHECK-clause hashing. These results
used a disposable WSL archive of the clean `1064b2e87` base with exactly the
eight files changed by `8cf1fb544` copied in. The archive's shell-script CRLF
line endings were normalized only in `/tmp` so Bash could execute them.
`8cf1fb544` was then merged with concurrent branch work and pushed as
`eeffa27a4` to `origin/add-double-entry`.

| Backend / scope | Command or method | Result and limits |
| --- | --- | --- |
| MariaDB 10.11, `8cf1fb544` code | `ECONOMIC_ACCOUNTING_DB_IMAGE=mariadb:10.11 bash tests/async/run_economic_accounting_schema_mysql.sh` | Fresh bootstrap, immutable migration run/replay, runtime compatibility, 10 accounting schema tests, 10 baseline schema tests, SQL authority and bank transaction stages passed. The later baseline transaction harness failed AddressSanitizer's leak check: 34,162 bytes in 438 allocations. The full wrapper did **not** pass; the source-snapshot stage was not reached. |
| MySQL 8.0, `8cf1fb544` code | Same wrapper, with the disposable archive stopped after baseline schema tests | Fresh bootstrap, immutable migration run/replay, runtime compatibility, 10 accounting schema tests and 10 baseline schema tests passed. Authority, transaction and source-snapshot stages were not run on MySQL. |
| Source and lifecycle contract, merged `eeffa27a4` | `python scripts/validate_runtime_compatibility.py`; `python scripts/validate_data_lifecycle.py`; `python tests/async/test_runtime_boot_compatibility.py` | Valid 220-table runtime/lifecycle inventory; nine boot-contract tests passed. The detailed fingerprint table lists in the C++ and shell queries are checked for parity. |
| Maintained build, `8cf1fb544` code | `make -s -C src CC=g++-12 -j2 BIN_ROOT=.../bin/plan5-runtime` in the WSL archive | **Blocked by local dependency:** `hiredis/hiredis_ssl.h` is absent in that WSL installation. Focused `g++-12 -std=c++20 -Wall -Wextra -Wpedantic -Werror -fsyntax-only` on the changed `src/sql/sql.c` passed; both changed C/C++ files passed `clang-format --dry-run --Werror`. This is not a server-build pass. |
| Writer registry, merged `eeffa27a4` | `python scripts/validate_economy_accounting.py`; `python scripts/generate_economy_writer_coverage.py --check` | Both contract checks passed, with 562 routes and 1,126 unmapped lexical sites. `coverage_complete=false` and release remains blocked. |

All SQL tests above used newly named loopback Docker databases. The wrapper
disposed its containers, and neither the configured database nor `.env` was
used. The baseline harness leak and missing local build dependency were open
at this checkpoint; the follow-up below tests both with the maintained build
environment.

## Follow-up qualification on `2002e7a05`

The baseline transaction harness now defaults to `g++-12`, with `CXX` available
for an explicit compiler override. The unchanged fault cases under WSL's
default GCC 11 completed functionally but LeakSanitizer reported 34,162 bytes
in 438 allocations. The same cases compiled with GCC 12.3 passed with leak
detection still enabled, including 1,150 apply and 567 replay allocation
faults. This is a compiler-dependent test result, not a waiver of sanitizer
checking.

| Backend / scope | Command or method | Result and limits |
| --- | --- | --- |
| MariaDB 10.11, `2002e7a05` code | Full `ECONOMIC_ACCOUNTING_DB_IMAGE=mariadb:10.11 bash tests/async/run_economic_accounting_schema_mysql.sh` | **Passed:** fresh bootstrap, migration run/replay, runtime compatibility, 20 schema tests, SQL authority, flatfile and SQL bank transaction harnesses, SQL/client-free baseline transaction with sanitizer, and SQL/client-free native source snapshot. |
| MySQL 8.0, `2002e7a05` code | Full wrapper with `ECONOMIC_ACCOUNTING_DB_IMAGE=mysql:8.0` | **Passed:** the same complete sequence, including the bank, baseline and source-snapshot stages that the earlier MySQL schema slice did not run. |
| Maintained MariaDB build, merged `196794731` | `docker build --target build --tag duris-plan5-build:196794731 --build-arg BUILD_JOBS=2 .` | **Passed** the Dockerfile's Ubuntu 24.04 development server and area-tools build under the maintained warning profile. Build context excluded `.env` and player data through `.dockerignore`. |
| Maintained flatfile build, merged `196794731` | `docker run --rm duris-plan5-build:196794731 make -s -C src PERSISTENCE_BACKEND=flatfile -j2 BIN_ROOT=/opt/duris/bin/plan5-flatfile` | **Passed** in a disposable container from the same source image. |
| Maintained MariaDB build, merged `c608f4d24` | `docker build --target build --tag duris-plan5-build:c608f4d24 --build-arg BUILD_JOBS=2 .` | **Passed** the Ubuntu 24.04 development server and area-tools build after the native player-item graph and shop trade source merge. This build predates the later shopkeeper/pet remote commits. |
| Writer census, merged branch after `c608f4d24` | `python scripts/validate_economy_accounting.py`; `python scripts/generate_economy_writer_coverage.py --check`; `python tests/async/test_economy_writer_coverage_contract.py`; `python tests/async/test_writer_sites_coverage_contract.py` | Contract and matrix checks passed after reanchoring 34 earlier source moves, classifying four retained shopkeeper-cash assignments as a separate flatfile recovery projection, and reanchoring seven further sites moved by the latest source merge. All 48 writer tests and 1,628 writer-site checks passed. The current candidate has 563 routes, 2,776 occurrences, 2,718 unique sites, 1,659 mapped and **1,059 unmapped**. `coverage_complete=false`; `--release` still refuses `writer has no executable evidence`. |
| SQL shopkeeper recovery census, merged branch after `15f5f3802` | Accounting validator, generated-matrix check, 48 writer contracts and writer-site coverage contract | **Passed** after reanchoring 39 moved SQL sites with unchanged source text and classifying four retained SQL keeper-cash assignments as a distinct recovery projection. The candidate has 564 routes, 2,780 occurrences, 2,722 unique sites, 1,663 mapped and **1,059 unmapped**; 1,632 writer-site checks passed. The SQL cash path still lacks selected revision and active-epoch publication proof. `coverage_complete=false`; `--release` refuses `writer has no executable evidence`. |
| SQL shopkeeper and migration source checks, merged branch after `15f5f3802` | `test_shopkeeper_save_runtime.py`, `test_shopkeeper_population.py` and `test_immutable_migration_runner.py` in WSL | The two shopkeeper C++ harnesses passed. All 14 migration-runner tests passed from a clean Git archive after normalizing only the archive copy of the CRLF `mysql_socket_bin/mysql` wrapper. These checks do not run migration `0041` against a database. |
| Concurrent site-mapping merge after `4ded7d758` | Accounting validator, generated-matrix check, writer-site coverage contract and full writer suite | Retained 57 additional auction, crafting and shop site mappings from the remote branch while keeping eight shopkeeper cash recovery sites in their separate flatfile and SQL projection routes. The matrix now has 564 routes, 2,780 occurrences, 2,722 unique sites, 1,720 mapped and **1,002 unmapped**; 1,689 writer-site checks and all 48 writer contracts passed. Release remains blocked. |
| Active-epoch guard and wider mapping merge after `338fa7ba7` | Accounting validator, generated-matrix check, writer-site contract, three targeted writer contracts and four compound-refusal tests | Retained the remote combat, encounter, ship, shop and crafting mappings, reanchored shifted shop/crafting sites, and kept the eight shopkeeper cash assignments in separate recovery projection routes. The matrix has 604 routes, 2,780 occurrences, 2,722 unique sites, 1,864 mapped and **858 unmapped**; 1,833 writer-site checks passed. The full 48-test writer suite and backend journeys were not rerun on this merge. Release remains blocked. |

The two full SQL runs used a fresh archive of clean `efd2186d7` with only the
baseline runner change from `2002e7a05` copied in. Shell line endings were
normalized only in that disposable WSL archive. Containers and databases were
removed by the wrapper. The later merged source receives a dual-engine rerun
below; these earlier passes do not prove complete gameplay routes, flatfile
restart/restore, native audit reconciliation, or
release readiness.

## Integrated migration and database qualification on `6eede581a`

The first clean `168594796` MariaDB run stopped at new migration `0041`:
its verifier expected width-free `COLUMN_TYPE` strings and SQL NULL in
`COLUMN_DEFAULT`. MariaDB 10.11 reports `int(11)`, `bigint(20) unsigned`, and
the text `NULL` for this nullable integer default. The verifier now checks
`DATA_TYPE`, the unsigned flag and both null-default representations; its
sealed manifest checksum was updated. The next run exposed runtime pins still
at migration `0040`. The runtime manifest and compiled header now pin `0041`,
its history checksum, and metadata fingerprints measured on both disposable
engines. No configured database was used or changed.

| Scope | Result and limits |
| --- | --- |
| Full `run_economic_accounting_schema_mysql.sh`, MariaDB 10.11 | **Passed:** fresh bootstrap, immutable migration run/replay through `0041`, runtime compatibility, 20 accounting/baseline schema tests, SQL authority, flatfile and SQL bank transactions, SQL/client-free baseline transactions with sanitizer, and SQL/client-free native source snapshots. |
| Full wrapper, MySQL 8.0 | **Passed** the same complete sequence on a separate disposable container and database. |
| Static/runtime contracts in WSL | `validate_runtime_compatibility.py`, all 14 immutable migration runner tests, and all 9 runtime boot compatibility tests passed. Changed C++ header lines passed `clang-format --dry-run --Werror` on a normalized temporary copy. |
| Maintained MariaDB server and area tools | `docker build --target build --tag duris-plan5-build:6eede581a --build-arg BUILD_JOBS=2 .` **passed** from exact code commit `6eede581a` under the Dockerfile's Ubuntu 24.04 warning profile. |
| Maintained flatfile server | `docker run --rm duris-plan5-build:6eede581a make -s -C src PERSISTENCE_BACKEND=flatfile -j2 BIN_ROOT=/opt/duris/bin/plan5-flatfile-6eede` **passed** in a disposable container from the same source image. |

Both full wrappers used a clean Git archive of `70a9c79e5` with exactly the
five subsequent source/manifest/test changes committed as `6eede581a` copied
in. Only non-immutable shell line endings were normalized in that disposable
archive to execute under WSL. The wrapper removed both test containers. The
tested source matches commit `6eede581a`, but these checks do not certify a
gameplay route, native audit exporter, flatfile restart/restore, or release
workload. A development database that already recorded the earlier `0041`
verifier checksum needs
separate migration-state assessment; none was used in these tests.

The later `68af73add` merge uses a stricter `0041` cash verifier with a
matching manifest and runtime history checksum. It also maps all 2,722
current lexical candidate sites into 854 named routes. On that merged tree,
the accounting validator, generated-matrix check, runtime compatibility
validator, nine boot contract tests, all 48 writer contracts and 2,691
writer-site checks passed. The dual-engine and server-build passes above
belong to `6eede581a`; they do not qualify the later merged source. The
release validator still refuses `writer has no executable evidence`.

## Exact merged database snapshot `178caab0c`

A clean Git archive of `178caab0c` ran the complete disposable accounting
schema wrapper against MariaDB 10.11 and MySQL 8.0. Both runs **passed** fresh
bootstrap, immutable migration run/replay through `0041`, runtime metadata
and history checks, 20 accounting/baseline schema tests, SQL authority,
flatfile and SQL bank transactions, sanitizer-backed SQL/client-free baseline
transactions, and SQL/client-free native source snapshots. The archive kept
immutable migration bytes unchanged; only non-immutable shell line endings
were normalized for WSL. Each wrapper removed its test container. The same
commit passed the Ubuntu 24.04 maintained MariaDB server and area-tools build
with `docker build --target build --tag duris-plan5-build:178caab0c --build-arg BUILD_JOBS=2 .`.
The maintained flatfile server build passed in a disposable container from
that same source image, with `PERSISTENCE_BACKEND=flatfile` and two build jobs.

The registry has 854 routes and maps all 2,722 current unique lexical sites,
but `coverage_complete=false` and `playable_release_status=BLOCKED`.
These database and build checks do not establish full native audit exports,
flatfile restart/restore, player-visible journeys, or release workload budgets.

## Focused authority journeys on `35a7ba70a`

A clean archive of `35a7ba70a` passed the inactive SQL shop lock fixture
against separate disposable MariaDB 10.11 and MySQL 8.0 schemas. Its cases
cover five shop actions, cash exceptions, stale or unknown cash, hidden
custody and native children, missing stock, equipped items and an inactive
epoch. The SQL shopkeeper population harness also passed its duplicate UID,
legacy UID, invalid UID, cleanup, cash and save cases on this source.

The sanitizer-backed flatfile typed bank root journey passed on this archive
after its runner was changed to default to `g++-12` with a `CXX` override.
WSL's default GCC 11 had stopped at compilation of the merged C++20 atomic
shared pointer, before any bank case ran. The executed GCC 12 journey covers
sourced starter grant, receipt retention, concurrent shared-bank updates,
tamper and forged-result refusal, crash recovery and 692 allocation faults.
These are isolated fixtures, not live player journeys. The full accounting
wrappers and both maintained server builds above belong to `178caab0c` and
were not rerun after the later shopkeeper and bank source commits.

## Earlier executed evidence

| Backend / scope | Command or method | Result and limits |
| --- | --- | --- |
| Contract and census, current checkout | `python scripts/validate_economy_accounting.py` | Passed contract validation: 13 fixtures, 549 writer rows, 2,759 lexical candidate occurrences. This does not qualify runtime coverage. |
| Contract and census, clean `cfd852ea5` checkout | `python scripts/validate_economy_accounting.py` and `python scripts/generate_economy_writer_coverage.py --check` | **Refused:** writer census drift and stale matrix after concurrent source changes. The Heavens-only focused source-contract test passed in this checkout. |
| Release gate, current checkout | `python scripts/validate_economy_accounting.py --release` | **Refused**, `writer has no executable evidence`. This is the expected blocked result. |
| Coverage matrix, current checkout | `python scripts/generate_economy_writer_coverage.py --check` and `python tests/async/test_economy_writer_coverage_contract.py` | Matrix checked with 549 registry rows and no supplemental candidate; all 46 source-contract tests passed, including focused item files through Heavens special procedures. Coverage remains incomplete; these are not executable gameplay proofs. |
| Audit fixture, current checkout | `python tests/async/test_reconcile_economy_accounting.py`; `python tests/async/test_audit_accounting_invariants.py` | 16 and 9 tests passed. Creation origins, exact before/after item revisions, and retired account terminal state are included. These are synthetic snapshots and operation fixtures, not live native reconciliation. |
| SQL coin component, current checkout | `python3 tests/async/test_coin_transfer_accounting.py` under WSL | One component harness passed. This demonstrates a balanced SQL accounting component, not a qualified gameplay route or flatfile equivalent. |
| Flatfile evidence and dispatcher, current checkout | `python3 tests/async/test_flatfile_accounting_store.py`; `python3 tests/async/test_economic_flatfile_dispatch.py` under WSL | Storage harness passed 85 injected commit/recovery write, sync, rename, remove and process-exit cases; dispatcher passed in SQL and client-free modes after its harness was updated for the current item route. These are isolated component tests. |
| Flatfile accounting gate, current checkout | `python3 tests/async/test_economic_accounting_flatfile_gate.py` under WSL | Passed schema-2 item movement, sourced room creation/retirement, exact item references, replay and journal recovery; unsupported coin variants were refused. This is an isolated harness, not a real player journey. |
| Lifecycle, current checkout | `python3 tests/async/test_persistence_backup.py`, `test_flatfile_backup_manifest.py`, and `test_account_erasure.py` under WSL | 29, 4, and 7 tests passed using disposable fixtures. WSL Python 3.10 required a temporary `hashlib.file_digest` compatibility shim outside the repository; the Windows Python 3.12 run cannot execute the Unix `fcntl` / `os.getuid` paths. |
| Lifecycle manifest, current checkout | `python3 scripts/validate_data_lifecycle.py` | Passed: 217 database tables and 34 non-database stores. Existing accounting SQL tables and flatfile evidence are registered in the manifest/backup path. |
| MariaDB 10.11, clean base archive | `ECONOMIC_ACCOUNTING_DB_IMAGE=mariadb:10.11 bash tests/async/run_economic_accounting_schema_mysql.sh` with a WSL Docker CLI shim | Fresh bootstrap, migration run/replay, runtime compatibility, 10 accounting schema tests, baseline verification and 10 baseline schema tests passed. The later SQL bank harness failed to link (`sql_pool_discard_connection` and `player_snapshot_repository_*` undefined), so the full wrapper did **not** pass. Container was disposed. |
| MariaDB 10.11, clean `95ae59ccc` archive | Same full disposable wrapper | **Failed before accounting tests:** migration `0036_economic_sql_activation_receipt` references a verifier committed as mode `100644`, so the migration runner receives `PermissionError`. After making only the temporary archive copy executable, migration processing advanced but runtime compatibility refused a normalized metadata fingerprint mismatch and stale immutable migration state. Neither run qualifies the clean head. Containers were disposed. |
| MySQL 8.0, clean base archive | Disposable schema-only slice of `run_economic_accounting_schema_mysql.sh`, ending after baseline schema tests | Fresh bootstrap, migration run/replay, runtime compatibility, 10 accounting schema tests, baseline verification and 10 baseline schema tests passed. This did not execute the authority, bank, baseline transaction or source-snapshot portions. Container was disposed. |
| MariaDB server build, current checkout | `make -s -C src CC=g++-12 -j2 BIN_ROOT=.../bin/plan5` under WSL | Passed with the maintained warning profile, including incremental rebuild after the coin-steal, numbered-quest, smelter and blackjack refusal guards. The default `g++` is 11.4 and rejects `-Wuse-after-free=3`; GCC 12.3 was selected explicitly. An initial build into the existing root-owned object tree also failed on permissions, so output was isolated under `bin/plan5`. Four small uninitialized-value fixes were needed in unrelated gameplay/UI files before the warning-clean build passed. |
| Flatfile server build, current checkout | `make -s -C src CC=g++-12 PERSISTENCE_BACKEND=flatfile -j2 BIN_ROOT=.../bin/plan5-flatfile` under WSL | Passed with the maintained warning profile, including incremental rebuild after the refusal guards. Generated artifacts stayed under `bin/`. |
| Synthetic audit size sample, current checkout | `/usr/bin/time -v python3 scripts/reconcile_economy_accounting.py /tmp/duris-plan5-audit-bench.json --limit 0` under WSL Python 3.10 | A 33,155,369 byte JSON snapshot (95,000 rejected roots and 95,000 receipts, no native holdings/items) returned zero exceptions in 0.82 s wall time with 193,220 KiB maximum resident memory. This is a development-host parser/reconciler sample, not the release-host 32 MiB mixed-authority budget. |

The MariaDB and MySQL tests used disposable Docker databases only. No
production database, `.env` credential, player data or operational migration
was used. A passed schema slice is narrower than an end-to-end backend pass.

## Route and workload coverage from the earlier shared tree

The matrix generated from the shared working tree has 549 registry rows,
including the formerly supplemental legacy auction settlement definition.
Its lexical scan has 2,701 unique
path/line/family sites, 1,547 mapped to current registry evidence and **1,154
unmapped**. Lexical sites are
candidates, not a count of real writers; each needs semantic classification or
reachability proof before `census_complete` can be true. The scan now includes
ship coffer mutations and case-insensitive direct SQL writes to ship/bank
tables. Ship hydration is recorded as a projection, while combat rewards,
coffer claims, insurance fallback, SQL saves and deletes need explicit
authority or refusal decisions. The newest source review found reachable direct
numbered-quest rewards, quest requirement consumption, reward-item allocation
and coin theft; each is now an explicit unqualified route. The coin-steal path
debits victim cash before a separate `ADD_MONEY` credit, so it now refuses an
active epoch before that debit. Numbered-quest economic actions now refuse
before requirement consumption or reward publication; tag/skill-only actions
can continue without rewriting NPC cash. The smelter now refuses recognized
coin and ore handoffs before its direct cash or item mutation. Blackjack now
refuses new wagers, game actions and periodic payouts while accounting is
active. These are source-order guards, not qualified SQL/flatfile gameplay
evidence. A prior gift to the quest NPC has already passed through its separate
give route before the quest callback. An unresolved blackjack wager from before
activation needs a quiescence and disposition policy; its periodic payout is
refused after activation, leaving the table state pending.

All direct indexed `cash[]`/`bank[]` assignments found by the current scanner
are now classified. One local `bank[6]` declaration was removed as a lexical
false positive; the SQL `load_bank` assignment fills only a temporary load
result. The shared `ADD_MONEY` and `SUB_MONEY` helpers are linked to their
direct NPC/live mutations, while shared-bank display publication is a distinct
projection and its unused single-denomination variant is a dormant candidate.
Committed wallet publication and SQL player-load materialization are now
separate projection routes with revision boundaries. The new-player flat-file
baseline read-back is separated from legacy character-file fallback loading.
NPC template parsing, conversion/scaling, PET_NOCASH clearing, copyover wallet
decode and the later legacy gold override have distinct source routes.

Two recovery findings need an authority decision. The legacy pet file writes
four denominations, but restorePetStatus reads and immediately zeroes them;
restorePet then calls convertMob, which recalculates template cash. The
current code does not establish whether those saved coins were an admitted
holding or how their loss/reissue is accounted for. Copyover captures gold in
both its legacy mob entry and generated-NPC state, then overwrites decoded gold
from the legacy entry in both recovery paths without checking agreement. An
empty generated state also leaves the other denominations from the template.
The matrix requires refusal of active-epoch publication until source identity,
complete wallet and gold consistency are executable checks. A second projection
finding is SQL login: sql_load_account_bank zeros the PC bank before its query,
and the nanny login caller ignores a failed load after placing the PC in a room.
The matrix marks that route blocked until a successful, revision-matched bank
read is proven before publication. These are audit findings; no domain mutation
was changed in this pass.

Every direct GET_COPPER/SILVER/GOLD/PLATINUM assignment, indexed cash/bank
assignment, ship-coffer assignment, and bulk coin mutation currently found by
the scanner is linked to a reviewed route. This includes provisional new-character
and new-NPC initialization, explicit service NPC sinks, SQL wallet/bank loads,
and the three CLEAR_MONEY call sites; the header macro itself is classified as
a definition only. The bulk scanner now focuses on coin arguments and pile helpers
instead of treating unrelated object memset/memcpy calls as money writers. Pile
appearance-only calls, live NPC wallet-to-pile clearing, room pile merging and
transient container put are distinct in the matrix. All 166 current shared
money-helper sites are now linked to routes. The 157 newly linked sites include
guild deposits and withdrawals, service fees, travel refunds, ship sales and
purchases, corpse wallet recovery, NPC theft, and special-procedure rewards.
Legacy auction offer, bid, pickup and settlement definitions, its rejected-credit
callback, and the old boon cash completion are marked dormant after an in-tree
caller search; helper prototypes
are nonwriters. These are source classifications, not executable refusal proofs.
All 160 current typed submission/builder sites are now linked. The 151 newly
linked sites include starter bank and item grants, spell/item admission,
crafting, auction/shop/collector dispatch, payments and lifecycle handoffs.
Header declarations and in-memory command builders are classified as
nonwriters; submission wrappers and gameplay callers retain separate routes.
The remaining unmapped lexical sites are in item lifecycle and publication
families. Their semantic review and release evidence remain incomplete.

The first item pass linked all 28 current `create_money`, `MakeScrap` and
`instantiate_object_template` lexical sites. Template construction and header
declarations are provisional/nonwriter; flatfile corpse coin materialization is
a recovery projection that needs source proof. Ship treasure-chest coin loot is
a potential issuance source. Transfer-wellness death and two special-procedure
shatter paths convert a live NPC/player wallet to a pile, so the old wallet and
new pile must reconcile as a transfer. Existing coin put/drop, corpse recovery
and scrap helpers carry the remaining linked sites. `read_object`, `extract_obj`
and item owner/publication calls still require a route-by-route review.

All 82 previously unmapped item calls in `src/cmd/actobj.c` are now linked.
Committed get/drop/give and pet handoffs are marked as live projections that
must verify the committed UID result before publication. Separate direct
legacy get/drop/put/give branches remain blocked until their UID movement is
typed. Eating, drinking, poisoning and junking have explicit consumption or
destruction routes; junk's fixed coin reward must be linked to the retired item
UID. Staff recovery of an item from an invalid location is a distinct operator
custody route. Weight and equipment relinks retain the same owner and require
slot/topology proof. The definition-only food-template scanner stays dormant.

All 84 previously unmapped item calls in `src/world/handler.c` are now linked.
Fresh prototype weight probes, refused creation candidates, rejected wallet-pile
staging and failed corpse-compaction staging are separated from admitted UID
retirement. Shared room, character, container and equipment link helpers are
unfenced custody mutation points until their caller proves an exact committed
owner transition. `extract_obj` explicitly does **not** retire durable custody
rows; its call sites include legitimate unload and rejected staging as well as
direct decay/destruction. Generic decay and character teardown remain blocked
until each admitted UID is distinguished from an unload. The committed corpse
callbacks are separately marked as projections requiring receipt and topology
proof. Two visible post-commit creation candidates remain unqualified: bone-pile
publication after corpse compaction and room-coin-pile publication during
resurrection. Each needs its source value, new UID and native result linked to
the same root before activation.

All 119 previously unmapped item calls in `src/cmd/actoth.c` are now linked.
The 90 forage branch/publication sites are one world-source food grant route;
the giant's tree publication is a separate room creation path. The doodle-only
forage probe never admits its object. Player steal has a direct fallback after
`submit_trusted_steal` returns false, so a rejected typed submission can still
move an item without its UID root. Potion and legacy scroll consumption,
donation transfer versus duplicate destruction, and room blood consumption are
separate sink/ownership routes. `do_quit` has no in-tree caller because
`CMD_QUIT` dispatches `do_camp`; its retained pre-save drop/extract branch is
classified as dormant, along with `do_old_descend`. Their lack of current
reachability is not a license to revive them under an active epoch.

All 47 previously unmapped item calls in `src/sql/sql_player.c` are now
classified. Player, locker, private-chest, corpse and saved-room-item loaders
are recovery projections whose selected UID and complete source graph still
need active-epoch proof; failed staged materializations are distinct from
destruction. The recursive locker loader checks its durable owner only when
the saved `obj_uid` column is nonempty, leaving a missing-UID row able to keep
the newly instantiated prototype UID. The shopkeeper catalog restore query
does not select saved UIDs at all and equips stock rebuilt from templates,
including derived stock; it is an unqualified identity-creation route, not a
retained-UID projection. These paths must refuse active-epoch publication until
source identity and selected custody can be proven.

All 40 previously unmapped item calls in `src/world/db.c` are now linked.
`read_object` and rejected reset candidates remain unpublished; room, NPC,
equipment and container placement are four separate creation destinations.
The B/P reset commands can select an existing container by object number,
including one already held by a player, so the destination owner and parent UID
must be established before a generated item is inserted. Replacing an occupied
NPC equipment slot is a same-owner live relink and has its own projection gate.

All 60 previously unmapped item calls in `src/classes/salchemist.c` are now
linked. Ingredient consumption, potion and poison output, furnace smelting,
encrustment and delayed enchantment have distinct source and sink routes;
temporary recipe and material probes remain unpublished. A successful
encrustment consumes the base item and jewel before publishing a new item. The
chaos-material pouch helper increments a generated-use counter without debiting
the pouch balance; if its subsequent object creation fails, that counter can
advance without a published item. That failure path needs its own operation
identity and reconciliation rule before the route can be enforced.

All 85 previously unmapped item calls in `src/guild/artifact.c` are now
linked. Boot restoration on either backend creates owned ground and NPC
artifacts from saved vnum/location data without selecting retained item UIDs;
NPC vnum alone does not distinguish individual NPC instances. Staff file import,
duplicate cleanup, timer poof and swap can remove or replace admitted copies
through direct object operations and later separate character/corpse or artifact
tracking writes. The periodic artifact-wars penalty drops player artifacts
directly after its timer update. These routes need exact source, UID and owner
proof before active-epoch publication. Display-only prototype reads and dummy
character unload are classified separately. Staff swap can leave a provisional
replacement in the global object list on failed preflight; the SQL binding
repair branch also uses a prototype after extraction and can extract it twice.
Those two defects are recorded as audit findings, not qualified writer paths.

All 30 previously unmapped unique item sites in `src/mob/mobact.c` are now
linked. NPC mage behavior can generate a corpse in a room without a death
source or prior UID. NPC thief, item-ranking and hunt behavior only move existing
weapons between carrying and equipment slots under the same NPC owner, but
their active-epoch projection still requires exact UID and slot-state proof.

All 46 previously unmapped item calls in `src/world/random.zone.c` are now
linked. Random-zone setup creates room fixtures, keys, chests, sigils, epic
stones and NPC stock; the chest can later move from a room to a spawned NPC.
Its chest filler loads `VOBJ_COINS` (vnum 3), assigns a generated amount to
`value[3]` and inserts that coin pile into the chest. This is world coin
issuance requiring a balanced source posting, even though the current lexical
scanner reaches it through the object constructor and publication calls rather
than the direct payload assignment. A level-potion chest branch is unreachable
under a literal `false`; the relic proc's player potion grant is reachable.
The random quest consumes a player's sigil and grants epic points and generated
items through separate direct actions. Labyrinth reset destroys some room items
and relocates corpses/artifacts to its entrance, while lab creation can grant a
fresh relic to an NPC. None has active-epoch root or replay proof.

All 37 previously unmapped item sites across `world_recovery_pipeline.c`,
`world_recovery_npc_items.c`, `world_singletons.c` and
`flatfile_corpse_restore.c` are now linked. A prior inventory row called NPC
gear reconstruction a projection, but its G/E reset path reads new template
objects by vnum/count and equips recovered NPCs with fresh UIDs; it is now an
unqualified creation route. Copyover room objects do restore saved item UIDs and
parent links, so they remain projections gated on selected snapshot and
authoritative graph proof. Singleton shopkeeper reconciliation can transfer
recovered stock between NPC instances, destroy produced duplicate stock, or
create missing produced stock; these effects have separate routes. Flat-file
corpse restore publishes retained room-item UIDs, but it constructs the corpse
shell from a fresh prototype without assigning a retained corpse `obj_uid`.
Staged cleanup and partial-publication rollback are classified separately from
durable destruction; the saved coin-pile materialization route now includes its
room publication call.

All 37 previously unmapped item sites in `src/magic/spell_corpse_lifecycle.c`
are now linked. The committed player-resurrection callback's room drop,
transient-item cleanup, corpse contents return and corpse removal are live
projections requiring exact result/UID checks. The older full and lesser
resurrection bodies still contain direct inventory movement, corpse extraction
and wallet-to-pile or pile-to-wallet steps. Primary-backend PC corpses enter
durable deferral, but staff NPC-corpse resurrection and any non-primary fallback
can reach the direct branches. Unmaking similarly defers PC corpses while its
non-PC path directly releases children and extracts the corpse. Corpse portal
also moves a live corpse between rooms without a typed owner result. The
legacy wallet routes now explicitly include coin-pile room publication and
extraction.

All 29 previously unmapped item sites in `src/item/enhance.c` are now linked.
Ordinary enhancement debits a fee, publishes a new item, then extracts the old
source and usually its carried donor; Chaos pouch mode retains the donor.
Modifier enhancement changes the source item's affected fields before its fee
debit and then consumes an essence. Superior enhancement debits cash, consumes
multiple materials and changes source fields in sequence. Its Chaos pouch
generated-use update happens after the fee and upgrade; failure only reports an
alert, leaving the upgrade in place without matching counter evidence. These
effects need one fenced root each. Template reads for base modifiers, material
names, target planning, descriptions and boot indexing are provisional.
Separate world-source grants issue turkey gear, NPC death essence and reset
fallback materials to NPCs.

All 31 previously unmapped item sites in `src/core/files.c` are now linked.
Serializer prototype reads are temporary, while `write_one_object` can assign a
UID to the passed live item when persistent-UID output is requested; that
assignment is an admission candidate outside a typed root. Flat-file terminal
save unloads the live inventory after authority save even if the account
character projection save fails and merely alerts. SQL terminal save stages
equipment, restores it on failure or ordinary save, and unloads after terminal
success. Legacy item restore and single-item decode can retain a newly generated
UID when the saved record lacks a UID flag, so active-epoch publication needs a
selected identity check. Pet save temporarily unequips and re-equips gear.
Compiled rent confiscation helpers have no in-tree callers; separate `#if 0`
child movement and pet extraction blocks are nonexecuting candidates.

All 27 previously unmapped item sites in `src/cmd/actmove.c` are now linked.
Breaking a tracked player key submits typed destruction and removes the live
object only in the committed callback, while the NPC or missing-UID branch
still extracts directly. Movement creates temporary Path of Frost ice in a
room, and opening a faerie bag or turkey innards grants a fresh random reward
before consuming its carried input. Rejected random templates and ice without
a valid room are unpublished candidates. All three lockpick break paths
extract held picks directly. Dragging an item, including a player corpse,
changes its room without an owner transaction. The latter direct mutations
need exact UID, source and room/owner proof before activation.

All 24 previously unmapped item sites in `src/item/storage_lockers.c` are now
linked. Enter, save, sort and rollback relink retained items through a
temporary locker character, chests and the room; those are projection
candidates requiring exact saved UID, locker/chest owner and coin-pile proof.
Chest fixtures are created from templates and later extracted, while their
destructor can spill remaining private contents into the room or limbo. A
new locker unconditionally extracts leftover objects in a reused room, and
locker exit moves room corpses to the exit room. An artifact found on the
locker floor is returned directly to the player. These are separate custody
routes, not evidence that all locker objects are temporary. The access check's
temporary restored character is unloaded without retiring selected custody.

All 18 previously unmapped unique item sites in `src/cmd/actnew.c` are now
linked. Combat disarm keeps a weapon under the same owner while changing its
equipment slot. Making a lock consumes a carried template after changing the
target container or door, and making a key changes an existing blank key and
container lock in place; those payload changes needed explicit routes because
the lexical item-call scanner does not see them. A held pick can break after
key shaping. Throwing a potion can destroy it, drop it in a room or consume it
after spell effects. The cast path unequips before the spell loop: an over-level
early return can leave a held potion detached, and successful remix retains a
carried potion. These are unresolved exact-UID and rollback requirements.

All 20 previously unmapped item sites in `src/magic/spell_conjuration.c` are
now linked. Active-PC branches for minor creation, flame blade, shield, food,
doom blade, mandrake consumption and sticks-to-snakes arrows use the shared
typed item owner, while NPC or inactive branches still mutate directly. This
conditional path is recorded in the matrix without counting the entire spell
as a qualified gameplay producer. Channeling directly creates and later
destroys a room avatar orb. Failed typed grant candidates are freed before
publication, and committed sticks-to-snakes arrows are removed from live
state after the batch result. The fallback arrow loop detaches selected arrows
first and stops consuming them if the victim dies early, potentially leaving
remaining detached objects. No spell path has a real player journey here.

All 19 previously unmapped unique item sites in `src/combat/range.c` are now
linked. Gather temporarily unequips and re-equips the same quiver, while its
arrows move directly from a room or corpse into that quiver. Firing removes
an arrow from the quiver and can give it to the target, drop it in a room or
scrap it; the already mapped scrap call remains a separate sink. A thrown
weapon moves from the thrower to a room unless it returns. Loading ammunition
changes weapon and missile counts and can extract the exhausted missile.
Partial loading keeps the missile UID with a lower count. These routes need
one source and exact UID, owner, parent and payload result before publication.

All 28 previously unmapped item sites in `src/guild/guildhall_rooms.c` are now
linked. Nine guildhall room classes instantiate and publish fresh door, board,
heartstone, window, fountain, counter, portal and tome fixtures. Each derived
`deinit` then calls `obj_from_room` and clears its wrapper pointer without
extracting or retiring the object. The detached object can remain in the
global object list at `NOWHERE`; subsequent room initialization can create a
new copy. The matrix requires an explicit transient-fixture exemption or an
exact UID creation and teardown policy before active-epoch use. This is an
audit finding, not a repaired lifecycle path.

All 40 previously unmapped item sites in `src/classes/necromancy.c` are now
linked. Six corpse-raise bodies first offer a durable deferral but retain
direct fallbacks that move children to a PC caster or summoned follower and
extract the old corpse. For PC corpses with contents, they also call
`create_saved_corpse`: `clone_obj` and `clone_container_obj` create a fresh
corpse shell and fresh child UIDs, then publish that duplicate graph in
corpse storage while the originals move elsewhere. The storage copy and its
timed cleanup require an explicit inaccessible-evidence policy or a linked
creation and retirement account. The committed raise callback's child moves,
transient/coin cleanup and corpse removal are separate gated projections.
Exhume and summon host directly issue fresh room corpses; wall of bones and
compaction have direct corpse/material sink paths when deferral does not own
the operation. Compaction can release contents before bone-pile allocation
fails, leaving the source corpse emptied. `spell_corpseform` returns with a
disabled message before its item body, so those lexical sites are unreachable
in this build.

All 40 previously unmapped unique item sites in `src/specs/specs.heavens.c`
are now linked. The slot machine grants two fresh restring coupons in its
jackpot branch before submitting the wallet payout, so those effects can
diverge on payout failure. Opening the treasure chest grants fresh potions,
then calls `obj_from_room` on the chest without extracting or retiring it;
`obj_from_room` leaves the object in the global list at `LOC_NOWHERE`. Io's
assistant creates a rose before a separate `do_give` handoff. A monolith
absorbs another monolith's charges and destroys its UID, and the banana proc
extracts the eaten banana before allocating a replacement peel. Holy and
good/evil swords have separate owner transfer, slot relink and destruction
paths. Registered trap, badge, prison object and gift-cap procs also destroy
live items directly. The flying citadel's apparent room transfer is behind an
unconditional return and is classified as unreachable in this build. These
routes remain blocked for active-epoch use until exact UID/source/root and
backend evidence exists.

All 49 current direct-SQL lexical sites are linked to named routes. Ten added
rows distinguish combat reward SQL, legacy auction pickup and compensation, collector SQL,
item repository custody/payload, corpse lifecycle, death restitution, snapshot
quarantine, and saved-item store/delete. Existing routes now link their SQL
subwrites, including auction settlement/claims, currency apply, SQL reset and
saved-item recovery retirement. These links describe the native writer and its
transaction boundary; they do not certify balanced postings or playable use.
The retained `auction_pickup_legacy` definition subtracts a pending claim before
submitting an asynchronous wallet credit; an immediate failure or later rejected
callback restores the claim with separate direct SQL. No in-tree caller reaches
that legacy definition, and it must remain closed unless exact claim, credit,
rollback and retry identity are proven under one root. The active pickup uses a
typed auction submission path, whose playable accounting proof is still pending.
Snapshot quarantine retains disputed item
custody; saved-item row deletion is not by itself item destruction. The SQL item
repository is a component of an owning item root and is not counted as another
schema-2 gameplay producer.

Eight matrix routes have a schema-2 gameplay producer and one SQL coin
component has balanced postings. That component has no qualified playable
dispatch/publication path. The matrix reports `coverage_complete=false` and
`playable_release_status=BLOCKED`; unsupported direct writers must refuse
under an active epoch. For per-route backend, authority, source/sink and
blocking details, use `writer_coverage_matrix.json`.

The sampled workload consists only of synthetic reconciliation fixtures,
component harnesses and disposable schema/backup fixtures. **Real player
journeys: none qualified.** No 1,000-root mixed workload, latency percentile,
storage-growth sample, checkpoint recovery time or release-host 32 MiB audit
memory/time measurement has been recorded. The development-host synthetic
audit-size sample above cannot certify the release-host mixed-authority budget.
The limits and measurement method are specified in
[AUDIT_OPERATIONS.md](AUDIT_OPERATIONS.md); the release performance gates remain
unverified.

## Read-only price audit increment (2026-09-27)

At source commit `ca0524c44`, the JSON reconciler reports a rejected root
carrying a realized price and refuses a negative, non-integer or out-of-range
signed 64-bit copper price. The bounded price view includes only committed
roots with valid copper values. `python tests/async/test_reconcile_economy_accounting.py`
passed 17 synthetic snapshot tests; `python scripts/validate_economy_accounting.py`
passed the contract check for 854 routes and 2,780 candidate sites. The
`--release` validator still refused with `writer has no executable evidence`.
These checks did not read a native SQL or flatfile snapshot and add no playable
route qualification.

## SQL EAB1 opening-origin extraction (2026-09-27)

Source commit `a6456307d` adds `scripts/economic_sql_audit_origins.py`.
`python tests/async/test_economic_sql_audit_origins.py` passed four exact
decoder/transaction refusal tests. The disposable
`tests/async/run_economic_sql_audit_origins_mysql.py` runner passed on
MariaDB 10.11 and MySQL 8.0.46 using a minimal InnoDB schema, a `SELECT`-only
audit account, the CLI output path and a corrupted-digest refusal. The CLI
refused to overwrite an existing output file. The test containers and schemas
were removed afterward. This proves only the EAB1 origin extraction slice;
it does not exercise a full upgraded schema, current native holdings or UID
authority, retained nonbaseline operations, or a complete SQL audit snapshot.

## Writer census after concurrent route changes (2026-09-27)

At census commit `678b7df5d`, the 854 route records reanchor all 2,722
unique lexical sites (2,780 occurrences) to the merged source. The moved
paid-practice debit is still classified as a legacy writer with an active-epoch
refusal harness; that harness passed all three tests under WSL `g++-12`.
`python scripts/generate_economy_writer_coverage.py --check`, the ordinary
accounting validator and all 48 tests in
`tests/async/test_economy_writer_coverage_contract.py` passed. The matrix
continues to report `coverage_complete=false` and `BLOCKED`: lexical mapping
does not prove all reachable writers or playable backend qualification.
`python scripts/validate_economy_accounting.py --release` still refuses with
`writer has no executable evidence`.

## Partial native SQL audit cut (2026-09-27)

At source commit `5255e5dce`, the read-only SQL exporter captures EAB1
origins, nonbaseline immutable accounting rows, mapped wallet/bank balances,
and current UID positions within one repeatable-read transaction. It always
sets `complete=false` and lists the native/history classes it cannot yet
attest. The guarded disposable runner
`tests/async/run_economic_sql_audit_snapshot_mysql.py` passed on MariaDB 10.11
and MySQL 8.0.46 with a minimal InnoDB schema and `SELECT`-only account. A
committed wallet-to-bank root reconciled with only the mandatory
`evidence_loss` marker. A concurrent wallet update after origin capture was
not visible in that cut. Injected missing-posting and stale-native-balance
cases produced their corresponding exceptions. The CLI wrote a bounded
diagnostic file, and the reconciler returned nonzero for its incomplete input.
All test schemas and containers were removed. This is **not** the full SQL
snapshot acceptance: unmapped native rows, coin piles, escrow/claims/treasury,
unlinked ownership events, postbaseline origins, retirement and cross-epoch
source scope remain unproven. No flatfile authority or real player journey
was exercised by this increment.

## SQL native mapping census increment (2026-09-27)

At source commit `efb44efb7`, the partial SQL exporter adds a database-wide
wallet/bank row census and counts rows with no active mapping in any lineage.
It also counts selected-lineage native rows with duplicate active mappings,
dangling mappings and mappings to rows with null balance or revision fields.
The reconciler reports bounded coded exceptions with full counts. These
diagnostics do not establish that an unmapped legacy row belongs to the
selected epoch, and the exporter still sets `complete=false`.

`python tests/async/test_reconcile_economy_accounting.py` passed 18 synthetic
tests; `python tests/async/test_economic_sql_audit_origins.py` passed four.
The guarded `tests/async/run_economic_sql_audit_snapshot_mysql.py` passed on
disposable MariaDB 10.11 and MySQL 8.0.46 minimal InnoDB schemas with a
`SELECT`-only audit account. Its new injected cases cover an unmapped wallet,
duplicate wallet mapping, dangling bank mapping and a wallet mapped to a
different lineage. The test containers and schemas were removed afterward.
`python scripts/validate_economy_accounting.py` passed the contract check for
854 routes and 2,780 candidate sites; `--release` still refused with
`writer has no executable evidence`. No full production schema, flatfile
authority, real player journey or release-host performance budget was tested.

After merging concurrent route work at integrated commit `b9c8a6a14`, the
18 reconciler tests, four origin tests and ordinary accounting contract check
passed again. `make -C src` in WSL could not run with its default `g++`
because that compiler does not recognize `-Wuse-after-free=3`. Retrying with
`CC=g++-12` compiled many units but stopped at the missing development header
`hiredis/hiredis_ssl.h` in `redis/redis_connection.c`. The integrated server
build is therefore unverified; this is an environment dependency failure, not
a claimed successful build or a diagnosed source defect.

## Lineage-wide source claim audit increment (2026-09-27)

At source commit `7864d4470`, the partial SQL exporter reads nonbaseline
source claims throughout the selected lineage and records the owning
operation's epoch, source and outcome. It counts committed source-bearing
operations without an exact claim and source values reused by multiple
committed operations across epochs. The reconciler accepts valid prior-epoch
claims and reports missing, reused, orphan or rejected-operation claims.
It still sets `complete=false`: baseline claim scope and policy-required
source events on other-epoch operations remain unproven.

`python tests/async/test_reconcile_economy_accounting.py` passed 19 synthetic
tests, including the earlier-epoch and malformed-claim cases. The guarded
`tests/async/run_economic_sql_audit_snapshot_mysql.py` passed on disposable
MariaDB 10.11 and MySQL 8.0.46 minimal InnoDB schemas with a `SELECT`-only
audit account. Its injected prior-epoch operation, missing exact claim,
reused source and orphan claim produced the expected results. The test
containers and schemas were removed. Four origin tests and the ordinary
accounting contract check also passed. The `--release` validator still
refused with `writer has no executable evidence`. No real player journey,
full SQL schema, flatfile audit or release-host performance budget was tested.

## Current dual-engine schema and replay qualification (2026-09-27)

Clean Git archive `0ba143f92` passed the complete disposable
`tests/async/run_economic_accounting_schema_mysql.sh` wrapper on MariaDB
10.11 and MySQL 8.0.46. Each run performed fresh bootstrap, immutable
migration run and replay through `0043`, runtime compatibility checks, 20
accounting/baseline schema tests, SQL authority checks, flatfile and SQL bank
transaction harnesses, sanitizer-backed SQL baseline transaction checks, and
native SQL source snapshot checks. Both wrappers exited zero and removed their
database containers. Only non-immutable shell line endings and the WSL Docker
CLI path were adapted in each disposable archive; the tested tracked source
and immutable migration bytes matched the named commit.

The first clean-archive attempt found that the `0043` verifier lacked Git's
executable bit. The next attempt advanced through schema and authority checks
but found that the SQL bank harness did not link the collector transaction
adapter used by its shared repository. Commits `3ddcc28fd` and `0ba143f92`
fixed those two test-path defects before the passing runs. These wrappers do
not exercise the partial audit exporter against a full active schema, an
upgrade of an existing installation, flatfile restart/restore, or real player
journeys. The integrated server build and release workload remain unverified.

## Writer census reanchor after concurrent source moves (2026-09-28)

At census commit `472b4b408`, 72 registry site entries and their matching
embedded census rows were reanchored to unchanged source lines in SQL player
recovery, special money helpers and item movement. Four writer-contract
assertions naming old SQL lines were updated. The full 49-test writer suite
on the preceding merged head exposed five stale-anchor failures; the five
affected tests then passed after reanchoring. The other 44 passed before the
edit and were not rerun afterward. `python scripts/validate_economy_accounting.py`,
`python scripts/generate_economy_writer_coverage.py --check` and
`python tests/async/test_writer_sites_coverage_contract.py` passed. The
registry retains 854 routes, 2,780 lexical occurrences and 2,722 mapped
unique sites. Its semantic `coverage_complete=false` and release `BLOCKED`
remain accurate: mapping a lexical site does not qualify a playable writer.

## Migration `0044` and shopkeeper restore census (2026-09-28)

The merged `0044` shopkeeper property change added one lexical `extract_obj`
site on a failed, unpublished restore candidate. Census commit `5c5711dbd`
classified it under `recovery.sql_shopkeeper_stage_cleanup`, reanchored the
moved SQL player sites, and refreshed the embedded census and matrix. The
registry now has 854 routes, 2,781 occurrences and 2,723 mapped unique sites.
Two focused writer tests, the ordinary accounting validator, the generated
matrix check and 2,692 writer-site contract checks passed. The matrix still
reports `coverage_complete=false` and release `BLOCKED`.

Clean Git archive `5c5711dbd` passed the complete disposable
`tests/async/run_economic_accounting_schema_mysql.sh` wrapper on MariaDB
10.11 and MySQL 8.0.46. Both runs completed fresh bootstrap, immutable
migration run and replay through `0044`, runtime compatibility, 20
accounting/baseline schema tests, SQL authority, flatfile and SQL bank
transactions, sanitizer-backed baseline transactions and native SQL source
snapshot checks. Both wrappers exited zero and removed their containers.
Only non-immutable shell line endings and the WSL Docker CLI path were adapted
in each temporary archive. These checks do not prove an upgrade of an existing
installation, complete SQL or flatfile audit export, playable route coverage,
or the release workload and server-build budgets on this merged commit.

## Integrated maintained builds (2026-09-28)

At merged source commit `428d1e4fc`, these commands passed:

```sh
docker build --target build --tag duris-plan5-build:428d1e4fc --build-arg BUILD_JOBS=2 .
docker run --rm duris-plan5-build:428d1e4fc make -s -C src PERSISTENCE_BACKEND=flatfile -j2 BIN_ROOT=/opt/duris/bin/plan5-flatfile-428d1e4fc
```

The first built the Ubuntu 24.04 maintained MariaDB server and area tools;
the second built the flatfile server from that exact source image. The Docker
build supplied the hiredis development header missing from the WSL host's
earlier direct `make` attempt. These are build results only; no playable
server journey, reconnect, fault replay or release-host workload budget was
qualified at this commit.

## Remaining release gates

1. Review the named routes beyond their lexical site mapping; attach
   executable evidence or explicit active-epoch refusal to every real route.
2. Produce complete, fenced SQL and flatfile native/evidence exports. Run the
   read-only reconciler against each backend after fresh install, upgrade,
   restore and injected evidence loss. A JSON fixture cannot attest its own
   completeness or operator access.
3. Keep the latest immutable migration verifier, runtime manifest and compiled
   schema contract synchronized on the eventual release commit; rerun both
   full disposable engine wrappers after later schema changes. Qualify flatfile
   journal interruption, restore, source/UID dedupe and receipt replay with
   actual domain roots. Resolve any later build or harness failure on that commit.
4. Repeat both server builds on the eventual release commit and run the
   focused gameplay/fault matrix for both backends, including live
   publication, reconnect and player-visible state.
   Measure the stated operation, latency, storage, checkpoint and audit budgets.
5. Publish a report for one exact integrated commit with the actual workload,
   route results and unsupported paths. Only then can a separate deployment
   decision be considered.

Correction and restitution submission are deliberately absent from the
read-only audit tool. A later operator writer needs authenticated authority,
expected-state checks and original-operation linkage; audit exceptions never
cause an automatic balance or custody adjustment.

## `finish-accounting` writer inventory checkpoint (2026-09-28)

On branch `finish-accounting`, based on `main` at `d686d4c70`, the source
inventory was reconciled with the current quest-offering and SQL shop code.
It now has 859 named routes, 2,784 lexical occurrences, 2,726 unique sites,
and zero unmapped current lexical sites. Removed direct quest extraction sites
were retired, five current quest/shop route entries were added, and moved
unchanged sites were reanchored. The generated matrix and normal contract
validator pass, as do the 2,695 writer-site checks, 49 existing route tests,
and two targeted new-route checks.

The `duris-refactor-builder:20260927` image (GCC 13.3) passed `make -C src -j4`
on this branch's unchanged C++ source. The first `make world` attempt failed
because the Windows checkout gave `areas/m_slow` a CRLF shebang. Four area
entry scripts now have LF checkout rules in `.gitattributes`; `make world`
passed in `duris-release-audit-runner:local`, which has the required world
tools. The focused durable quest offering and shared movement publication
retention harnesses passed in that audit container. These are build and
focused-harness results, not a cold-restart quest or live SQL gameplay pass.

This is source inventory evidence. The release validator still refuses
`writer has no executable evidence`; `coverage_complete=false` and
`playable_release_status=BLOCKED` remain the correct report values. The next
qualification work is the reproducible build and quest-reward restart fault
from [the active plan](FINISH_ACCOUNTING_PLAN.md), followed by the remaining
gameplay, native audit, backend, and workload gates above.

## `finish-accounting` quest and save checkpoint (2026-09-28)

Issue #8's zero-source-ID quest grant was reproduced from its code path. Commit
`cde63f95e` derives a stable, nonzero, signed-safe item reward source from the
consumed offering UID, reward VNUM, and duplicate ordinal. The source-ID and
durable-offering focused tests passed. In the disposable `duris-finish-accounting-qa:local`
container, an ordinary Human Druid gave three items to a quest NPC. The actual
flatfile server granted one reward blade; a save and cold server restart kept
the same reward UID and did not restore the offerings. The standalone journey
passed from a freshly compiled cached artifact (`SERVER_BUILD built
build=477.420s lookup=25.206s`). This establishes the normal grant and
restart path on that fixture, not SQL qualification or interruption recovery.

The manual `run_quest_reward_ack_crash.py` journey then stopped the server at
`complete_quest_offering`, which is called after the offering publication
acknowledgment. On a cold restart the offerings stayed consumed but no blade
existed. Its `--confirm-loss` baseline passed; its default recovery expectation
remains a failing acceptance case. The callback context and reward obligation
are not yet durable across that boundary, so Step 1 and the release gate remain
open.

Issue #10 was traced read-only on the isolated main-branch playtest instance.
The latest rejected player-save journal snapshot contained 23 items while the
SQL custody and saved-item projections each held 24. The missing snapshot UID
was a VNUM 393 bandage; `do_bandage()` directly extracted it without retiring
its active custody row. No live player data or database rows were changed during
this diagnosis. The save guard correctly refused a destructive checkpoint.

Commit `e75e78311` changes the durable bandage path to submit a typed item
destruction before removing the live object or starting healing. The separate
untracked legacy branch remains direct. A fresh disposable MariaDB 10.11
journey with an ordinary Human Druid and an incapacitated NPC passed: exactly
one starter bandage UID became a destruction tombstone, its saved-item row was
absent, `save` acknowledged, and a cold restart preserved that state. The SQL
server built with GCC 13.3; the player custody guard, 15 live movement contract
cases, movement prompt runtime, and smith tradeskill tests passed. This proves
the new path for fresh actions. The already stranded playtest character still
needs an individually reviewed, owner-authorized data repair or recovery path;
the running main-branch instance has not been modified or restarted.

## Main integration checkpoint (2026-09-28)

PR #9 and PR #11 were merged to `main` as `e92a5424b` and `ab423482a`.
Follow-up source commit `0ceb561fe` keeps the normal quest grant source bound
to the consumed offering UID and changes the bandage branch to select durable
custody per item. Its publication explicitly marks the player's inventory
dirty after removing the committed bandage. The follow-up adds executable
quest and MariaDB bandage save/restart journeys and source contracts.

On that integrated source, the maintained GCC 13.3 MariaDB server build passed.
The flatfile server built from the same checkout (`SERVER_BUILD built
build=426.082s lookup=30.064s`), and its ordinary quest offering, reward
grant, save, and cold reconnect journey passed with the reward UID retained.
The disposable MariaDB bandage journey passed with one consumed UID, a
destruction custody tombstone, no saved-item row, and successful save after a
cold restart. The quest source and durable-offering tests passed. The writer
registry maps all 2,728 current unique lexical candidate sites across 861
routes; the normal accounting validator and generated-matrix check pass.
The release validator still stops at `writer has no executable evidence`.
`coverage_complete=false` and `playable_release_status=BLOCKED` remain correct:
the post-offering-ack quest crash still loses the pending reward, and most
writer routes still lack executable release evidence. The existing stranded
playtest character has not been modified.

After merging `main` into `finish-accounting`, the default
`run_quest_reward_ack_crash.py` acceptance case was run against the integrated
flatfile binary. It failed as expected: `expected 1 reward after ack crash,
found []`. This is an observed recovery failure, separate from the passing
normal quest cold-restart journey. Quest completion must retain a durable
reward obligation through publication acknowledgement before Step 1 can pass.

## Quest continuation format checkpoint (2026-09-28)

`finish-accounting` now encodes a bounded, typed quest continuation in item
transfer payload version 9. At offering submission, it snapshots the player,
quest and mobile identifiers, room VNUM, completion time, consumed root UIDs,
and ordered reward goal types and amounts. The decoder still accepts versions
2 through 8; the focused compatibility harness rejects truncated or oversized
continuations, mismatched player/root identities, and continuations attached
to unrelated movement reasons. The durable-offering harness verifies the
captured reward terms. The ordinary accounting validator and generated-matrix
check pass with 861 routes and all 2,731 current unique sites mapped.

This is a format and capture checkpoint, not a recovered quest completion.
The SQL and flatfile authorities still need to retain the obligation after
offering publication acknowledgement, record effect receipts, and replay it
without the original NPC or quest catalog. The post-ack crash acceptance case
and the release validator remain blocked until those paths pass on an
integrated binary.

## Flatfile quest obligation checkpoint (2026-09-28)

The flatfile item ownership catalog now stores the validated quest continuation
in the same authority image as a successful offering destruction. Catalog
format 6 keeps earlier versions readable. A pending lookup returns the exact
operation ID and reward terms after a fresh load; an acknowledgement marks the
obligation complete with an idempotent catalog update. The repository harness
proved that a stale, rejected offering creates no obligation, a committed
offering survives catalog reload and command replay once, another player cannot
acknowledge it, and acknowledgement survives another reload. A catalog-format
5 fixture with an existing operation still loads and replays. The maintained
server build and flatfile repository harness passed.

The SQL authority still needed the same atomic obligation at this checkpoint.
Neither backend had a player-ready reward executor with durable per-effect
receipts. The flatfile acknowledgement API was not called by gameplay yet.
The post-ack process-crash acceptance test therefore remained a known failure.

## SQL quest obligation checkpoint (2026-09-28)

Migration `0045_quest_reward_obligation` adds a retained obligation row keyed
by the offering operation. The SQL item transfer transaction inserts the exact
validated continuation after successful custody mutation and before the inbox
result and commit. A rejected offering inserts no row. Apply, duplicate replay,
and explicit reconcile verify that the retained bytes and player identity
match the original command. The migration is additive and re-runnable; the
runtime table inventory, lifecycle registry, and pinned MySQL 8.0/MariaDB
10.11 metadata fingerprints were updated from disposable engine measurements.

Both disposable engine schemas applied all 45 migrations, passed each new
migration verifier, passed the schema-only runtime compatibility check, and
accepted an idempotent migration rerun. The focused MariaDB item transfer
harness passed a stale quest offering with no obligation and a committed
offering with exactly one custody ledger entry and exact persisted reward
terms through duplicate apply and reconcile. The maintained GCC 13.3 server
build, 14 immutable-migration runner tests, 9 runtime-boot contract tests,
51 writer-route contract tests, and 2,699 writer-site checks passed. The
writer matrix still records the accounting release as
`BLOCKED`; this checkpoint stores the SQL obligation but does not execute or
acknowledge reward effects after reconnect. The post-ack crash acceptance test
still needs a player-ready replay path with per-effect deduplication.

## Quest obligation recovery-read checkpoint (2026-09-28)

The version-1 continuation now has a bounded decoder shared by transfer
validation and the flatfile pending/acknowledgement reads. It checks version,
exact length, player and quest fields, root UIDs, and reward terms before any
pending work is exposed. A SQL worker-side reader loads at most 64 pending
obligations for one player, verifies the committed inbox result and exact
continuation, and fails the whole read on malformed or excess rows. A fresh
MariaDB connection found the committed offering and its reward terms in the
focused transfer harness; an intentionally malformed row was rejected without
overwriting the caller's prior result. The v2-v9 transfer compatibility test,
flatfile item repository harness, and maintained SQL and flatfile server
builds passed.

The default `run_quest_reward_ack_crash.py` was rerun against this flatfile
binary. It still failed after cold restart with
`expected 1 reward after ack crash, found []`: the offerings remained consumed
and the item was absent.
This directly confirms that repository lookup alone does not close the
publication-ack crash window.

No player-ready executor consumes these reads yet. Version 1 also lacks a
frozen zone-story definition and party/XP context, so it cannot by itself
prove the full quest status and group reward outcome after a catalog or party
change. Reward effect receipts, safe acknowledgement, and the post-ack crash
journey remain open; the accounting release decision stays `BLOCKED`.

## Quest acknowledgement worker checkpoint (2026-09-28)

The SQL idempotent acknowledgement repository call now runs through a bounded
worker queue. The worker starts after world boot, drains acknowledgements on
the persistence pulse, and stops before the SQL pool shuts down. Flatfile
acknowledgements use the same queue and execute their catalog write off the
game thread. Failed acknowledgements leave the obligation pending and emit a
redacted persistence alert. Both the maintained SQL server and flatfile server
builds pass with this worker enabled.

The worker has no gameplay producer yet: player-ready loads do not submit reward
effects, and successful item publication does not enqueue acknowledgement. No
player-ready reward executor consumes the loaded terms. Consequently the
post-ack process-crash acceptance test remains failing and the accounting
release decision stays `BLOCKED`.

# Plan 5 modern room observer build and original seed recipe handoff

The current published native source builds a complete SQL server with its existing
room recovery observer, but the original room seed recipe fails at link time.
There is no qualified seed binary and no modern room SQL cold-boot result from
this slice. The independent raw room-payload reader remains open. Component
checks and a compiled server do not establish release completion.

## Delivery, preservation and exact source

- Branch: local and remote `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base: `d1d942897e5bb1a19c8c12f2a0d1db311c37bf0b`.
- Refreshed primary: `86bef1dba80f5658eb234b0816fff4f3ca0cc43d`.
- Frozen composed tree: `a130c2d8588e0721e21ed909957ed87c72bca5be`.
- Native tree: `833d3085815b396861ad18a77635412212381e4b`.
- Canonical64 migration tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.
- Frozen archive SHA256:
  `5be7b238e700dc096418088a7acf834b8a8b43bb14fc13f7b183ea1817a86d1f`.
- Result commit and verified remote tip are recorded after publication in
  `D:/Dev/Tests/Duris/accounting-plan5/room-recovery-20261008/delivery/result.json`.

The composed tree contains the refreshed primary plus the same 11 explicit Plan5
overlays used for the preceding saved-handoff diagnostic work. `source.json`
records every overlay blob and every archived file's body, mode and link target.
There are no native, recipe, contract or migration overlays. The six primary
changes since `ab56edfc9d7cb84641833cb89d53f71ddbed8217` are documentation only;
their raw checkpoint bytes and diff are retained under `primary-checkpoint/`.

All seven earlier Plan5 branch tips were independently verified as ancestors of
the delivery branch before this work. Publication verifies them again:
`7d2f8e8153f637c38e19054cf1202436d3f9a28a`,
`36355900e9cdf28e83413b5171cce2d43a3b9864`,
`be854d0dfab3b6012eef8b542fa883ba115db731`,
`32d829ef2c3010e523e2afac8772c0c56ef9c5b8`,
`d0e35336892e866eb2ecbad1c7f31ab6ccaeefa2`,
`2e93b9f69dde19ecfee520b6c9eed39aedd33e66` and
`42141f6268707787b44d3b95bab2290ae3898b87`.
[Branch consolidation](PLAN5_BRANCH_CONSOLIDATION_2026-10-05.md) preserves the
original slice/source facts. All continuing handoffs use this same remote branch.

## Owned files and shared boundary

This report and the additive
[remote follow-up](PLAN5_REMOTE_FOLLOWUP_2026-10-06.md) are the only repository
changes. No production code, shared native recipe, coordinator, contract,
producer, registry/matrix, migration or activation file is edited. No shared API,
schema or payload-format change is requested.

The concrete shared recipe maintenance request below belongs to the primary.
The user declared the primary-local shared notebook nonblocking. This report,
raw evidence seal and post-push delivery receipt form its curator packet;
application, acknowledgement and primary adoption are not claimed.

## Executed checks and evidence

Evidence root: `D:/Dev/Tests/Duris/accounting-plan5/room-recovery-20261008/`.
Helpers: `D:/Dev/Temp/accounting-plan5-room-recovery/`.
Builds: `D:/Dev/Builds/Duris/accounting-plan5-room-recovery-20261008/`.
All shell launches used the owned worktree explicitly and TEMP/TMP on D:.
Docker bound D: outputs directly; frozen source and strict scratch were on private
RAM filesystems. Every container used network `none`, with no database or Redis
service started in this slice. Existing jobs, containers and volumes were kept.

The immutable image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Tools are GCC13.3.0, Python3.12.3, GNU nm2.42 and mysql_config10.11.14.
`seal/native-build-inventory.json` retains their executable hashes and versions.

| Check | Result and scope |
| --- | --- |
| Original `make world BIN_ROOT=/workspace/bin` | PASS, 1.368s in the final build stage; complete generated world/lookup files retained |
| Full production-profile SQL observer build | PASS, 482.752s; 754 fresh compilation commands, 754 objects and 754 dependency files; zero warnings/errors |
| Original real-pool room seed compile | FAIL at native link, 175.108s; 41 distinct unresolved symbols; no eligible executable produced |
| Original native payload codec/admission test | PASS, 7.454s, ASan/UBSan; no services and no SQL proof |
| Original native placement/cleanup test | PASS, 0.383s, ASan/UBSan; exact restored state and unrelated sibling UIDs |
| Original room journey `--self-test` | PASS, 0.033s; 22 negative parser cases; no SQL/server execution |
| Maintained object provider census | PASS; every unresolved symbol mapped, 21 actual provider source files; no corrected closure tested |

The exact entry commands are:

```text
make world BIN_ROOT=/workspace/bin
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=mariadb BIN_ROOT=/workspace/bin EXTRA_CFLAGS=-DDURIS_SQL_ROOM_ITEM_RECOVERY_TEST
python3 -u -B tests/async/run_economic_sql_real_pool_mysql.py --family room --compile-only /workspace/bin/tests/room-seed --compile-budget-seconds 600
python3 -u -B tests/async/test_sql_room_item_payload.py
python3 -u -B tests/async/test_sql_room_item_publication_runtime.py
python3 -u -B tests/async/run_sql_room_item_payload_recovery_journey.py --self-test
```

The three component scripts execute unchanged through `runpy`, with transparent
subprocess observation and retention before their original temporary-directory
cleanup. `components/commands.json` retains both actual compiler commands and
both executable invocations, including all original sanitizer flags. Temporary
sources/executables and native inventories are under `components/temporary/`.
`DURIS_TEST_SANITIZERS=1`, ASan leak/halt and UBSan halt/stacktrace settings were
set. There are no SKIP results and no altered assertions, flags or deadlines.

The full server SHA256 is
`1ea8f0d1b1885b9272ac417dba2ed58706a80d4e0cf947312d5fde4bd6ce68bf`
(223,463,320 bytes). It is retained as `build02/dms_new` and in the D: build
directory. This is an observer-enabled build, not a deployed binary or service
qualification. `build02/terminal.json` binds 1,318 repository dependencies and
28 external headers. Frozen source bodies/modes/link targets pass before and
after every executed stage. `components/terminal.json` independently records
its terminal success (81.737s total including symbol census).

The original complete-world generator used the real Ubuntu Noble
`dos2unix_7.5.1-1_amd64.deb`, SHA256
`c743df55dfe9c58f211c96b1046a57957583b80febad235d081ce62c569546cc`.
The package, generated world and lookup outputs are retained. No stand-in world
generator or smaller modeled native oracle was substituted.

## Established shared recipe defect and narrow handoff

The unmodified `compile_family("room", ...)` derives its source list from
`tests/async/run_item_transfer_schema_mysql.sh`, replaces the item harness with
`tests/async/sql_room_item_payload_mysql_harness.cpp`, and adds `account_load.c`
and the actual `sql_pool.c`. At the pinned source, that list cannot link current
consumers in `critical_command_repository.c`,
`economic_sql_item_transfer_transaction.c`, `economic_command_admission.c`,
`item_transfer_command.c`, `item_transfer_accounting.c`, `auction_repository.c`
and `shop_item_runtime_payload.c`.

`build02/seed-build.log` preserves the original link failure and traceback.
`components/linker-provider-handoff.json` contains all 41 complete demangled
signatures and their real compiled providers; none is unmatched. Immediate
provider groups are:

- Six auction accounting/context codecs plus auction bid, listing, settlement,
  item-claim, money-claim and retained-verifier SQL transactions.
- Native mobile birth command/result codecs and its SQL transaction, including
  original birth observation and retained verification.
- Native quest coin-give and cost codecs, held-retirement recovery and lockpick
  continuation validation.
- The original held-retirement/native-quest checkpoint owners in
  `src/player/player_save_pipeline.c` and
  `find_recovery_object_template(int)` in `src/world/db.c`.

Request: refresh the primary-owned source closure for the original item/room
native recipes and keep the real implementations and original fixture behavior.
The 21 immediate providers are evidence, not a claim that blindly appending them
supplies a complete closure. In particular, native world-template and checkpoint
owner dependencies need the primary's actual fixture/runtime boundary; invented
fallback stubs or relaxed unavailable-state assertions would not qualify it.

Preserve these exact interfaces/invariants:

1. `compile_family.sources`, room harness replacement and original real-pool
   defines/wrappers still select genuine coordinator/pool/COMMIT observers.
2. Its `inputs` map must hash all actual compilation and included harness/header
   dependencies. Keep strict C++20 warnings, ASan/UBSan, original budgets and
   immutable executable destinations/SHA256 qualification metadata.
3. `DURIS_SQL_ROOM_ITEM_SEED_EXPORT=1` must still export only after real journal
   ACK, coordinator/pool shutdown and the fixture's historical transition. No
   global activation or smaller manufactured seed is an acceptable substitute.
4. The observer server must retain actual publication and runtime-identity
   markers, original complete world prerequisites, exact graph equality and
   unchanged durable state across both cold boots.

Consumers are the original real-pool item/room families, the optional retained
room verifier build and `run_sql_room_item_payload_recovery_journey.py`.
Required follow-up tests are the original item/room compile modes, the retained
room verifier variant, and the unchanged default room journey on both isolated
MariaDB and MySQL schemas with canonical64 migrations and pinned binaries.
No failure is asserted for the coin family: it was not compiled in this slice.

## Preserved failure, unrun gates and independent follow-up

The first build attempt passed the world generator but used my invalid
`PERSISTENCE_BACKEND=sql` argument. Make refused before compiling; its terminal
exit1 and raw log remain under `build/`. The corrected `build02/` is separate.
That stage has terminal exit1 because seed linking failed after the full server
build passed. It is not relabeled as a passing batch. Components and the sealer
have separate terminal exit0 records. All four containers are stopped.

The original MariaDB/MySQL room journeys, both cold boots per backend, active-drop
producer variant, native authority variant and planned audit negative controls
are UNRUN. The prepared `journey.py` is retained as an unexecuted observer helper;
there are no database cuts or negative-control results to import. The compile
failure is the concrete blocker for this original journey, not a blocker for
other independent Plan5 work or the user's shared notebook.

Source inspection confirms that the current independent exporter captures UID
positions and coin payloads but only uses ordinary `sql_room_item_payload`
presence to fence legacy saved rows. Complete bounded literal room-payload
capture, original proof binding, modern/historical/current-season classification
and independent room graph diagnostics remain to implement and natively qualify.
Retained sidecar presence alone must not establish current room custody.

The primary checkpoint's private 111-production/23-fixture/five-schema NPC/current
money and mobile-P candidate remains source-reviewed and unexecuted. This report
does not test or import that candidate. Native callback installation/recapture,
complete value/UID/origin/forest correspondence, original producer/player/fault/
recovery and backup/retention/erasure continuity, both-backend qualification and
full R1-R8 remain open. Prior slices keep their original source-qualified scope.

## Raw seal and curator facts

`seal/evidence.json` SHA256:
`3fe6f4a440aad9afc1a81c578566c5ec27a5dcf0da5a4c6b56a6e68fe59904b1`.
It binds 1,620 files, 1,934,757,548 bytes and 1,537 native build files, with zero
copied links/reparse points. Native file bodies were cross-checked against the
read-only Linux inventory before sealing. Docker inspection, raw consoles,
commands, frozen archive, all failures, component artifacts and compiler/provider
evidence are retained. Delivery is sealed separately after the report commit.

Curator update: retain this exact successful observer build and three component
results; add the original seed-link defect and primary-owned recipe request;
leave both SQL journeys, raw room audit, callback and release gates open. Keep all
earlier branch work and follow-up on remote `codex/accounting-plan5`. Accounting
was not activated; there was no production data access, audit autocorrection,
deployment, PR merge or direct push to `experimental-accounting`. Wallet-root
ITEM_MONEY exclusions and the declined inactive spell-path change are preserved.

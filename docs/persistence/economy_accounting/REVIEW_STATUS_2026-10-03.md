# Experimental accounting review status - 2026-10-03

The goal remains active. Full R1-R8 implementation and qualification are open;
accounting activation and release are BLOCKED. Earlier source-specific results
remain in [October 2 status](REVIEW_STATUS_2026-10-02.md). The
[completion plan](FINISH_ACCOUNTING_PLAN.md) and
[requirements](REMAINING_REQUIREMENTS.md) retain the feature contract.

## Flatfile native quest-alias erasure milestone

The actual whole-account menu previously reported successful deletion while
retaining the deleted PID's zone-story quest aliases. Whole-account deletion
bypassed core/files.c's ordinary runtime erasure. Native character deletion now
prepares a validated shared quest-state rewrite under the borrowed authority
lock and commits it with the other character operations. Public quest-state
load/save use the same authority lock and recover its journal first. Missing
state is permitted; malformed, stale-catalog or incomplete serialization refuses
before commit. The maximum transaction has 18 operations. After successful
account erasure the runtime refreshes its quest tracker; a failed reload disables
publication instead of writing the old cached names.

This reuses existing ordinary-character erasure semantics: targeted character
state, telemetry and containing quest transactions are removed, and X erasure
markers remain. It does not qualify the protected quest-history retention policy,
all personal stores, typed economic erasure, or retained economic identity.
Those broader R8 requirements remain open.

Qualified combined native source: 27c8bb637b42ebbdd59b3fd84a2b1a00a3457288,
on published remote prefix 3b5247aa9a5a0a9453ad700cae19ebc44346647f (Telnet PR
691). All 1,224 tracked native files match the frozen source at
/opt/duris-accounting-flat-alias-telnet-review/source. Strict production SQL,
flatfile and pfile builds pass. SQL SHA-256:
aac22665cb53aa5ae6ca385096fac1f6e172675d8d856fc0f58f5bea5467c3e6.
Flatfile SHA-256:
49630831bb9f0147c4ddd7c9dcc9603e1ade7c297df99b08b4c7a9705617f95d.

Qualification:

- All three actual flatfile menu journeys pass. Both uncertain and durable
  publication faults prove permanent fencing, non-cancellable retry, persistent
  recovery refusal, pending-journal SIGKILL/restart, exactly-once account/player
  erasure, a later same-process state write without alias resurrection, and a
  second cold restart. Ordinary character deletion proves unchanged refusal,
  playable repair/retry, alias erasure and usable account after cold restart.
- Native ASan/UBSan corrupt-state refusal/repair and all 18 journal interruption
  boundaries pass on source 0bbfb55c21f5a091a22bc8c9afc947a39aa41cbc. Fresh
  recovery preserves unrelated PIDs and repeated retry is byte-identical.
  All native inputs remain identical in the combined source except net/comm.c
  and net/mccp.c, neither linked by the erasure inspector. The reused inspector
  SHA-256 is 00ef552d022b84173919c4eda896e5b716c137eae47aecb1ebd967a5d06ab450.
  This is unchanged-component evidence, not a relabeled whole-source build.
- Actual SQL character-deletion admission/refusal/rollback/retry/cold-restart
  compatibility journeys pass on both MariaDB and MySQL with the combined
  SQL binary. These do not exercise whole-account SQL alias erasure.
- All ten native recovery cases pass in 672.350 seconds, including both engines'
  dump/import, schema/history/value qualification and isolated service boot.
  This duration under qualification load is not a measured workload budget.
- Incoming Telnet framing/fragmentation/escaped-IAC/size boundaries, 14 terminal
  type cases and GMCP checks pass. The full formatter, deletion contracts,
  manifest, 54 writer contracts, 14 fixtures, standalone site contracts and
  generated matrix check pass. Exact signature reanchoring preserves policies
  and evidence: 864 routes, 2,815 occurrences, 2,756 unique sites, zero unmapped;
  751 runtime/projection proofs remain incomplete.

The first observer fixture reused bound defaults and recreated the old account;
the next used an account name rejected by normal name validation. Explicit
valid observer arguments correct both fixture defects. Earlier failed logs are
retained; only the final combined-source journeys qualify this milestone.

## Latest remote integration

Remote prefix e265b8338 added owner-deadline character maintenance while the
milestone was qualifying. The unpublished alias commit rebases without source
conflicts. Integrated native source: 836eceeb4d69be626e82d4ff9de4050fa99e5ebe.
The preceding evidence stays pinned to 27c8bb63 and its unchanged components;
it is not relabeled as qualification of the new maintenance integration.
The maintenance integration subsequently passes both strict production builds,
pfile, full formatter and owner tests at source 836eceeb4d69be626e82d4ff9de4050fa99e5ebe.
The SQL-erasure source below also qualifies the combined gameplay/recovery paths.
The older evidence remains pinned to its original sources; release remains blocked.

## Next established defects and open gates

## SQL whole-account quest-alias erasure milestone

Real native SQL probes established successful account/player deletion while
retaining aliases, and deletion despite malformed quest state. The SQL owner now
locks and validates the shared singleton inside its existing deletion transaction,
erases all captured PIDs across seasons, and rewrites state before destructive
native writes. Missing singleton state is permitted; failed queries, malformed or
stale catalogs, allocation/serialization failures and failed writes roll back.
The helper borrows the caller's transaction and uses the real current season.
MYSQL_RES and escaped-string allocations are released on exceptions. Successful
whole-account deletion refreshes the runtime cache on SQL as on flatfile.

Qualified native source: ce7550d67b3bb5bc7a8467a5a3e03eccf77d5ded on remote
prefix c79e3ed429584b255bd72bb5fc37393b1c1d4c8a. All 1,226 native files match
the frozen archive at /opt/duris-accounting-sql-account-erasure-review/source.
Strict production SQL, flatfile and pfile builds and full formatter pass.
SQL SHA-256: a1086dd4492bb3464bc6d1a5953216da3561794fd5a201c00ec3354d7d0e9e4b.
Flatfile SHA-256: 7dd07a864329d70617a4e0d7afe9c459a2ab30cbe58e3237cf2bda7f83f69a32.

- Actual MySQL and MariaDB whole-account journeys pass malformed/stale state,
  quest-state write refusal, late native rollback, permanent fence/repaired retry,
  all-season alias erasure, unrelated PID and X-marker retention, exactly one
  acknowledgement, later same-process cache write and cold restart.
- Actual character-deletion compatibility journeys pass both engines' admission,
  soft-delete/late-cleanup refusal and rollback, playable retry and cold restart.
- All three flatfile menu journeys pass both publication faults and ordinary
  deletion, pending-journal crash/recovery, non-cancellable fencing and retries.
  Fresh ASan/UBSan erasure-inspector compilation and all 18 journal boundaries
  pass at this source after the maintenance header change. All three menu
  repeats pass with the fresh inspector. SHA-256:
  ec8187ae5006e1ad704dfd7fe66a0d4068c010022f7974f6eb5107a0a0966b94.
  The first combined run reused the older inspector; it is retained separately.
- All ten native recovery cases pass in 350.772 seconds, including both engines'
  full dump/import, schema/history/value checks and isolated service boot.
  This is bounded recovery evidence, not a measured workload budget.
- Native component strict-warning and ASan/UBSan/leak checks pass 387 allocation
  failures, including failure after escaped-buffer allocation, exact locked reads,
  failed-query versus missing-row behavior, erasure and unchanged-state retry.
  An initial WSL transport failure remains unavailable evidence; the retry passed.
- Incoming maintenance, periodic rearm, Telnet/TTYPE/GMCP owner checks pass.
  Census signatures are unchanged; line-only registry/assertion reanchoring,
  54 writer contracts, 14 fixtures, standalone site and deletion contracts pass.
  coverage_complete=False; release BLOCKED.

This applies ordinary quest-erasure semantics, including deletion of containing
quest transactions, not a new protected-history retention policy. Full personal
store erasure, typed economic identity/retention, lost-COMMIT reply and cache-load
failure qualification remain open. The existing real connection-loss exclusion
owner guard is retained; no production bypass is claimed from a same-session
failure hypothesis. SQL ships.money omission is established as the next bounded
independent-audit issue; its fix is separate from this native milestone.

The frozen published 0be1cdf30 broad run finished 848 tests: 829 passed,
11 skipped and eight failed in 11,112.060 seconds. Account recovery, area-coin
pickup and creation-prompt journeys failed the unchanged 600-second server-build
deadline. Full-world subsequently reused a verified artifact but failed the
unchanged 180-second player-authority-inspector build deadline before gameplay.
Copyover custody, launcher, game-loop phase and spell-schedule harness failures
need current-source investigation; their historical failures remain retained.
This run excludes subsequent alias, fence-ack, Telnet and maintenance changes.
It is not a current-source broad pass. A separate older full-world attempt on
native 0bbfb55c failed the 600-second server-build deadline.

Reliability review confirmed failed artifact builds discard completed objects.
The separate cache repair now passes 11 real Make/compiler tests for atomic
object/dependency publication, exact-key resumed work, source changes, concurrent
publication, caller cancellation including SIGKILL, path refusal and preservation
of a running executable. Full native/full-world checks are underway at source
ce7550d6 with the candidate helper. Default 600-second/-j2 and strict warning
gates are unchanged; no failed attempt is converted to a pass and resumed builds
are not cold-build performance qualification.

Captured staging-generation clone/rollback/mount proof remains blocked on the
requested backup location. Complete writer/compound-gameplay/native audit,
protected retention, workload, full-world and integrated-current-source gates
remain open. Inventory and synthetic/component checks do not satisfy them.
coverage_complete=False; release BLOCKED. No inactive spell-path, production
activation/data, migration checksum, timeout or release policy was changed.

Local ignored evidence SHA-256:

- tmp/flat-account-alias-red.local.log: 126b24b5eb1ec5654d7d30f25ccb61a381ee77c4edc1ba008195c0ba4e06a7b4
- tmp/flat-account-alias-final-build.local.log: 4ca405aeeb6ae2c7cdf9305dfc33abae3caa7ade3a8ca6d66ce4fe70566d0bb0
- tmp/flat-account-alias-final-native.local.log: 81fabadb3fb19068e807a781dc09cc1c94c90901049be456fc17ba1c87e4c24b
- tmp/flat-account-alias-final-recovery.local.log: ae3b9e987fb216cad59ad17d9c5ad5450c55f9e0cc34660727158ce8b884fd4c
- tmp/flat-account-alias-final-menu-journeys.local.log: 27179248e7719f69a5a629dcdfeb8f0a6a3a39871bfc1dc6623c75512e9a29b9
- tmp/flat-account-alias-final-full-world.local.log: dec65b8c2365c9b39355f77bb5e035ec5b26cb17a495a8a8b97e66ea1aceef45
- tmp/flat-account-alias-telnet-build.local.log: 6580f7e0f70d2e1d92e459398fc5f54855647bad14c2e7231ca5d04d998c9439
- tmp/flat-account-alias-telnet-pin.local.log: efb2f2f71cdb4381b7332e0b0a232b7aaa0aafd8881415fabb19db84ed2eca3b
- tmp/flat-account-alias-telnet-component-pin.local.log: a5b786c2ba0840e50ccb6c5ef5e7cf8bad3ef57d90de17aeb0fe8e1a3dc5d879
- tmp/flat-account-alias-telnet-owner-final.local.log: 9529d0f7d3379805e4e7c56c4b1a0f02447b67c8ddd37cbceefba11f7005de2a
- tmp/flat-account-alias-telnet-contracts.local.log: 0104e8758cb82494ba007712586eb604699edccc148cb0b97722de790e2d4243
- tmp/flat-account-alias-telnet-menu-journeys.local.log: 29b1b4d642554a3cba4efd3f813df8536fe7c8563a8e6e0a6593f45eb68be1fe
- tmp/flat-account-alias-telnet-sql-journeys.local.log: d0a88e8acd9fe59c1feb58b1082b19638e46f5823a7674a5b4880c62631abd06
- tmp/flat-account-alias-telnet-recovery.local.log: ea6cfb7b0356f89c84bdc5dff9f9843c908040247f412b95329b52ad813c3f68
- tmp/sql-account-alias-red.local.log: 4013b213b730ebb36ff97175b7cdbd7f27b29a5b247faaeba5c7154e9b6188df

SQL milestone local ignored evidence SHA-256:

- tmp/sql-account-alias-red.local.log: 4013b213b730ebb36ff97175b7cdbd7f27b29a5b247faaeba5c7154e9b6188df
- tmp/sql-account-erasure-before-actual.local.log: 3faff17fadc3af48463f0b0f1ef630773030f5d3ae34f8612b0b1d3de7ac2ef5
- tmp/sql-account-erasure-build.local.log: 2e5c7103a9ab3d515ee8fe419fcdbe9a3d2d6aeec31ca13ef118a163bc7ef291
- tmp/sql-account-erasure-formatter.local.log: f2efc2eb69dda6ceacbfd2534a79f615c4760f829f5ca75ab436cab8f61cb646
- tmp/sql-account-erasure-final-write-fault-journeys.local.log: 82963aa74e255852afb2dda7f6625c06c4830c542cd5755a12e39f4753d81154
- tmp/sql-account-erasure-character-journeys.local.log: d0a88e8acd9fe59c1feb58b1082b19638e46f5823a7674a5b4880c62631abd06
- tmp/sql-account-erasure-flat-menu.local.log: 29b1b4d642554a3cba4efd3f813df8536fe7c8563a8e6e0a6593f45eb68be1fe
- tmp/sql-account-erasure-recovery.local.log: 4c54a5047e0b171018d80b6cdb75c5aa0fc19dfad7cb620647e68cec429592e1
- tmp/sql-account-erasure-owner.local.log: edf11dd2531193c641f73e557702f727186e76239c6981eb7490a99859e716af
- tmp/sql-account-erasure-contracts-final.local.log: 1590fae54f3d824338fcf2e1474552a2b8fbe2a2ad5469efe15f12ee176c4ecc
- tmp/flat-account-alias-maintenance-build.local.log: aeaaaf5d03684341bd0f54c5357c488dbe8f1aa68ad4ed3d55e37bd04f353365
- tmp/flat-account-alias-maintenance-owner.local.log: 308ed3dfde88390bc85f2e269b7600de6e3efdc00c13b4aa2dc14e7eceb47846
- tmp/qualified-0be1cdf30-broad.local.log: 6d436c4040a6f470c80868c4ff0dbc29b1d676e782d3fe7611c21c8af977095c
- tmp/qualified-0be1cdf30-results.local.json: be4344babd47970d4765e28e42fec81692acc0a56ce85f1c4e124f716e096125

Fresh native erasure evidence SHA-256:

- tmp/sql-account-erasure-inspector-pin.local.log: ce6c6109964b9a27bd86b30d04fbf9ba14250048c7af2b4d36f4ba5457dde95b
- tmp/sql-account-erasure-flat-inspector-native.local.log: d08aed62a026acff767dd9226dc2af9a6fcb4211b654c5666a36113cd5d950d1
- tmp/sql-account-erasure-flat-menu-fresh.local.log: 29b1b4d642554a3cba4efd3f813df8536fe7c8563a8e6e0a6593f45eb68be1fe

The inspector dependency comparison initially found the changed maintenance
header; that refusal triggered the fresh build and repeat above. It is not a
failed production erasure test.


## SQL ship-coffer audit omission milestone

The disposable native-SQL exporter regression reproduced missing ships.money
while persisted coffers changed. The SELECT-only consistent cut now captures
all ship IDs and exact nullable signed-INT copper values separately from mapped
holdings. Ships join required InnoDB sources. Independent reconciliation validates
identities/value bounds/uniqueness and recomputes coverage, reporting unsupported
native authority and missing monetary revisions even for zero-valued rows;
NULL remains unknown and negative value is reported invalid. No account key,
revision, lineage or owner alias is inferred.

Both actual MySQL and MariaDB SELECT-only probes pass positive/zero/NULL/negative
capture, value change, missing/non-InnoDB source refusal and exact restoration,
concurrent-writer read-view separation, CLI export/reconcile including detail
limit zero, unchanged mapped holdings/economic evidence and exact pre/post reads.
Five new independent coffer tests, 61 reconciler tests and ten exporter tests pass.
The first extra CLI fixture reused an existing output path and correctly hit
exclusive-create refusal; the repaired fixture uses a separate new file and both
engines pass. This was a fixture failure, not an export overwrite policy change.

Native source remains ce7550d67b3bb5bc7a8467a5a3e03eccf77d5ded, published in
9f9d21132df1f30594454349c3d26c0225dd9eaf. This milestone changes diagnostic
Python only; native build/gameplay/recovery proofs remain at their original
source. Ship gameplay/enrollment, guild holdings, complete native authority/
writer qualification, protected retention, full-world/captured clone and workload
remain open. backend=sql_partial; complete=false; release BLOCKED.

Ship milestone local source/evidence SHA-256:

- scripts/economic_sql_audit_snapshot.py: 01bfb40618fe9eebaf3851c7bdc3cde6c00f7427cd48043ebef7bd009bb284c3
- scripts/reconcile_economy_accounting.py: 5509777c3286f46499cf18480fc4de36ddea5284a3a885a3ec78e8e81d52e8dd
- tests/async/run_economic_sql_audit_snapshot_mysql.py: 5a160e15a6659b003e44f2a05880ca8a4a4c749c3f217e9a5e4a47e7fe9084d0
- tests/async/test_ship_coffer_audit.py: 24f6e5320244725da406dcaba2bb356c853542faae7d3fb51d1a3ec9bce620d1
- tmp/audit-ship-coffer-red.local.log: e500904dab653c6c9bde3c6f54cc245aaff838429465dfe8b23ce160d292718f
- tmp/audit-ship-coffer-green.local.log: 15321b41ca8e657492e907546d6d6ee19b2c2132a5a07b5d17cc21bf8fb29955
- tmp/audit-ship-coffer-final.local.log: 4d8f7fcaeda84243829d62bac38c526961716ebefc4e2fa6c6af5eb46330e188
- tmp/audit-ship-coffer-cli.local.log: 15321b41ca8e657492e907546d6d6ee19b2c2132a5a07b5d17cc21bf8fb29955


## Launcher fixture watchdog dependency repair

The current launcher regression reproduced the frozen-suite failure: its
isolated project omitted scripts/game_loop_watchdog.py even though cycle_mud.sh
requires it for configuration validation. The fixture now copies the actual
watchdog dependency and also proves NaN stall configuration refuses with status
78. Existing port/configuration/secret/backup/database-independent checks pass.
Production scripts and native source ce7550d6 are unchanged. This is focused
fixture proof; the frozen broad failure remains retained and current-source
broad/full-world/workload gates are open.

- tmp/current-flatfile-launcher-baseline.local.log: 9fc0dfd0d8ecabcd4b6b671cf09680f18ef68000fba30f7e8e96ab75c22d4d5c
- tmp/current-flatfile-launcher-green.local.log: 3ee7202c269659c76b32f0de39beaa5c53d1dbaae226eb258b5975da9a65bdda
- tests/async/test_flatfile_launcher.py: 4c6bb06e6708df7e107095e8ead76b86bbb5c71355e7358d953cca4a505c3ec2


## Incoming persistent-transport integration

GitHub advanced to 4e6922b600ba56d4fe2ff77b4bcc9cbd495dd20c (PR 692)
before the launcher milestone push. The unpublished launcher commit rebases
normally; cache and spell-fixture WIP were preserved. The upstream launcher
fixture now includes the same real watchdog dependency, so the duplicate copy
was removed and this milestone contributes the invalid-setting regression.
Integrated native source: 4180f74573c8a4cfe669c057c36f9c36dfc6d7fa.
Prior native/gameplay/recovery evidence remains pinned to ce7550d6 and earlier
sources. Fresh transport-inclusive builds, owner/copyover/gameplay/recovery,
line anchors and current-source broad qualification remain pending. The running
full-world cache candidate also remains at ce7550d6; no evidence is relabeled.

The launcher fixture passes again on integrated native source 4180f745,
including invalid-watchdog refusal, after the normal unpublished-commit rebase.
- tmp/current-692-flatfile-launcher-green.local.log: 3ee7202c269659c76b32f0de39beaa5c53d1dbaae226eb258b5975da9a65bdda (LF-normalized)
- tests/async/test_flatfile_launcher.py: d559d11eca23b804556f3a4107b37d638bd3464ec3818dfbcae1fd3ef474af9e (LF-normalized)


## Spell-schedule failure fixture queue initialization

The current production-extracted schedule-failure harness reproduced the frozen
strict-warning compile failure after txt_q acquired four accounting fields. The
fixture now initializes the complete queued-command state and verifies bytes,
entry count and overflow flags remain intact across rejected scheduling.
Strict warnings, ASan and UBSan pass before and after the PR 692 integration.
Player-facing spell behavior and native source are unchanged; the declined
inactive spell-path change is not retried. These focused harness passes do not
replace current-source gameplay/broad/workload qualification.

- tmp/current-spell-schedule-baseline.local.log: 9583dd51a9f40c2cd46697afee2cb4a7227c533568c3fd41d7a9fac6b4ce5cc3 (LF-normalized)
- tmp/current-spell-schedule-green.local.log: 9bd0442ec1aedadd38f305356c08392d6ba91a8d9e2fe02010a5e607f05b2f8f (LF-normalized)
- tmp/current-692-spell-schedule-green.local.log: 9bd0442ec1aedadd38f305356c08392d6ba91a8d9e2fe02010a5e607f05b2f8f (LF-normalized)
- tests/async/test_spell_schedule_failure_runtime.py: 7fc107145f6766662d0b4588ba7ff6b7e7086e08996da15b5ea2d22f3c242436 (LF-normalized)

Current-source game-loop phase assertions and copyover custody already pass
without edits on source ce7550d6; the latter exercises real save/exec/recovery,
six publication faults and decode/materialization rollback under sanitizers.
These earlier component proofs are not relabeled after transport integration.
- tmp/current-copyover-custody-baseline.local.log: 72e020ec616d579b33f5c2877bebd943003df63bdb74b2219dc9075587fd23eb
- tmp/current-game-loop-phase-confirmed.local.log: 31448a14c3ec710f22d04214dbb62aff42727c0d5a86d159daaff162fede212b


## Interrupted-build artifact reuse repair

The verified-artifact helper no longer discards completed compilation work on
an unchanged-input timeout. Private exact-key workspaces retain only confirmed
complete object/dependency pairs; compiler output is staged and atomically
published after success. Changed-input or unconfirmed dead-owner work is
isolated/discarded. A watchdog pipe and inherited lock stop compiler descendants
when the caller exits, including SIGKILL. Pending work never qualifies as a
published artifact. Successful builds copy a fresh immutable executable and
hashed log, preserving already running executable inodes.

Existing artifact checks and eleven real GNU Make/installed-compiler tests pass
same-key resumed compilation, partial output refusal, changed inputs, concurrent
publication, timeout/caller TERM/SIGKILL cleanup, path/lock refusal, strict flags
and running-artifact preservation. The unchanged default 600-second deadline,
-j2 and strict warning gates remain. There is no internal retry loop, fake
metadata or deadline waiver. Attempt logs are retained.

The full native strict-production flatfile server builds in 452.463 seconds on
source ce7550d67b3bb5bc7a8467a5a3e03eccf77d5ded, with helper candidate.
Immutable artifact SHA-256:
103b44d0dda3059fc20bbe1027daef2834750de7fdb0e822082b6fae23059865.
The actual full-world player/floor-item save, authority readback and process-
restart journey then passes. This is one native build and journey; it does not
qualify all workload budgets, captured staging generations, the newer transport
integration or full R1-R8. Frozen broad failures remain unchanged. Fresh transport
source 4180f745 build and qualification are underway; initial cache-copy setup
failed before compilation and the continuation preserves the same frozen source.

Cache milestone LF-normalized source/evidence SHA-256:

- tests/async/server_build_artifacts.py: f0b456929ab6b9a7b2f7c8f3caf52feed2849ca2579aae4059bcd815db942e7d
- tests/async/server_build_compiler.py: 13764ca38d439f11a38eedf125dd4135123e4bbde1cf5389aeb6101b6965e51d
- tests/async/test_server_build_artifacts.py: 8f4e46b7612f49f77c9b9d13088400cde985b78633b7d85f3c129008b13f45d8
- tests/async/test_server_build_artifacts_resume.py: 0779635d56c0aa18ccb6298070c0e1ff991541e4f5917b7c62f243bdb0e09d7d
- tmp/sql-account-erasure-cache-full-world.local.log: a8daa8840feec5bbb4d6c4376297be48f4d43b77f260510a419baeade7a177e0


## Persisted guild treasury audit capture

The SELECT-only SQL cut reproduced a missing native.guild_treasuries collection
on real MariaDB. It now captures all guild IDs and exact four unsigned-INT
money values, including zero and UINT32_MAX. IDs remain reusable native locators;
outcome_revision is excluded because ordinary deposits/withdrawals do not
advance that prestige/construction revision. Guilds joins the required InnoDB
sources (18). Raw candidates remain outside mapped holdings and no account key,
lineage, origin or monetary revision is invented.

Independent validation recomputes bounded coverage and reports unsupported
native guild authority and missing monetary revisions for every row. Malformed
vectors, aliases, fake revisions, duplicate IDs and forged counts refuse.
Both MySQL and MariaDB pass SELECT-only value-change, fixed outcome revision,
missing/nontransactional source refusal, exact restoration, concurrent-writer
read-view isolation and CLI export/reconcile with detail limit zero. The first
independent run caught a missing exception-detail guild locator; the final
implementation safely emits the native ID. All 81 focused tests pass, including
five guild tests, five coffer tests, 61 reconciler tests and ten exporter/origin
checks. Failed attempts are retained.

Native source remains transport-integrated 4180f745; this diagnostic milestone
does not change gameplay or native persistence. Guild enrollment, money
revision/lifetime, deposit/withdraw atomic roots and recovery, full native audit,
protected retention, current broad suite, captured clone and measured workload
remain open. backend=sql_partial; complete=false; full R1-R8 release BLOCKED.

Guild milestone LF-normalized source/evidence SHA-256:

- scripts/economic_sql_audit_snapshot.py: 33868861e79f466cdf7fe3631f80c149b421722fae91749bd3a3a227718d106a
- scripts/reconcile_economy_accounting.py: 5a6a279ed376c008b3dc4609c38d904262011761d1fe12e967c2e6bdcb451abe
- tests/async/run_economic_sql_audit_snapshot_mysql.py: a640edc204bfb34f85519582eb50f917d7028aefe83d1507129489f6a1feddf1
- tests/async/test_guild_treasury_audit.py: 8624627ebc372008f10b059da358d1c425c72825b4567c265aa85ea305e8d75e
- tmp/audit-guild-money-red.local.log: efdf9e7a2cac4b62810ce2a9c5b2de3b47349cd6dcf5c41564333ef057bf2ae4
- tmp/audit-guild-money-green.local.log: 189e97c579fcaa5b290b3dd5c65d6ec42ccad60564e4888d1127210fdc3246b1
- tmp/guild-treasury-focused.local.log: 92634f8a58687761906ab1a8d2a84f4ad4c777b2074778665d7e766ca64f9ccd
- tmp/guild-treasury-focused-final.local.log: 8d7cdad19b122fd409a8572a5317020815a9107b1274b98d51984fff247ecbc5
- tmp/audit-guild-money-final.local.log: 189e97c579fcaa5b290b3dd5c65d6ec42ccad60564e4888d1127210fdc3246b1


## Queue extraction harness transport boundary repair

The frozen 4180 queue owner failed strict compilation before sanitizer runtime:
production-extracted functions referenced transport_frontend_input and
transport_world_finish_pulse without their declarations. The harness now
includes production net/transport.h. Its existing disabled-feature inline
implementations match this fixture configuration; no fake transport doubles,
queue assertions, deadlines or production behavior changed. Strict warnings,
ASan and UBSan now pass all bounded session queue runtime regressions.

The frozen incoming integration focused manifest records eleven other owner
passes and the original queue failure. Persistent protocol/readiness/Telnet/
WebSocket components, listener contracts, phases and 24 watchdog process cases
pass. The repaired queue result is separate evidence; actual transport,
readiness, accounting gameplay and R8 recovery are still under qualification.
Native source remains 4180f74573c8a4cfe669c057c36f9c36dfc6d7fa.

Queue milestone LF-normalized source/evidence SHA-256:

- tests/async/session_queues_runtime_harness.cpp: b32a0e8cb94965833d02ea268e6b253a22ec1c7f5ec748f46ae6853489e934ab
- tests/async/test_session_queues_runtime.py: 5f0799a51af578654a62b910ad7735693082ba22081fe019126d1ad02610c751
- tmp/transport-4180-focused-summary.local.json: 2b93ef67fa94bdcad76cf5246518e3fc0a126650136038b665b62df6037c6bb7
- tmp/transport-4180-session_queues_runtime.local.log: 1572f5981e864a1d6a3af199eb3fed4c8bca422ec2b5518506d5e95aad61e33b
- tmp/transport-4180-session-queues-green.local.log: 9e97152ff5b6423370edf9302e4d6a6e96d243ca154cdff4a11c6b5b100f2125


## Transport-integrated native qualification

Integrated native source 4180f74573c8a4cfe669c057c36f9c36dfc6d7fa passes
strict production SQL, flatfile and offline pfile builds plus full formatting.
All 1,229 archived native source files match the qualified workspace. Executable
SHA-256: SQL 54834b1568cf2fd2def39f80dff8370f664c37a7e684e5d7ee1ba0d2bbe7c0b1;
flatfile 60e9bc740ccbf4de0e44a1f636da5537483a0e57c98bebbe711346f3d66ddd40.
The inspector is freshly compiled against this source in 83.260 seconds;
earlier inspectors are not relabeled.

The real persistent-transport matrix passes authenticated Telnet/TLS/WebSocket
exec, compression/framing/order/deduplication, startup/exit/retry failures,
mixed authenticated sessions, orderly close, invalid input isolation, stale
barrier/retry, disconnect/reconnect, frontend failure, modified handoff refusal,
account-mutation barrier, bounded flood/backpressure and launcher watchdog.
Watchdog verifies delegated world pulses, single lifecycle delivery, exec,
cold restart and stalled-world detection. Listener measurements pass all
existing limits: p95 first-byte 3.164 ms, TLS handshake 7.127 ms, WebSocket upgrade
1.708 ms, 96-client burst 98.652 ms; measured idle CPU 0%, capacity 256 with extra
connection refused, and median plain/TLS command spacing 250.840/250.822 ms.
These are isolated test-server measurements, not mixed accounting workload.

All ten native recovery cases pass in 545.753 seconds, including both MySQL and
MariaDB full dump/import/private service boot, pending flatfile/legacy replay,
WAL/quarantine refusal and locker/spell receipt qualification. These are
fresh source-4180 proofs, distinct from earlier ce755 results. The frozen broad
suite at published 70aaa41384cb40ae786d06241ec5313ccafef822 is running;
no broad PASS, captured-staging-generation, full writer or R1-R8 completion is
claimed.

The incoming source preserves every lexical census identity and multiplicity:
2,815 occurrences/2,756 sites/864 routes. Only source line anchors and matching
contract assertions were refreshed after checking added/removed sets are empty.
52 writer tests and 14 golden fixture contracts pass; 751 runtime/projection
routes remain unqualified. coverage_complete=false; release BLOCKED.

Source review also confirms typed coordinator coin/item dispatch already
exists; Plan 1's bank-only starting-point text is corrected. Bank has actual
wrapped post-COMMIT reply-loss proof. Coin/item fixtures synthesize ambiguity
after repository success and use replacement SQL pool interfaces. Production
pool contention/lifecycle and ordinary drop/give/pile publication recovery are
still open, including room payload durability after ACK. No production or
inactive-accounting behavior was changed by this qualification milestone.

Transport integration LF-normalized evidence SHA-256:

- tmp/transport-accounting-integration-build.local.log: 13d8fe903ef08b8ada7480b04b908caa66c3eb969db64029d1b22fcb47867670
- tmp/transport-accounting-integration-build-continued.local.log: 9fa7e6a6966950ffe9fe8a0c499c4dd52c577e7cb0f9994ac9fcd3674af74891
- tmp/transport-accounting-integration-formatter.local.log: f2efc2eb69dda6ceacbfd2534a79f615c4760f829f5ca75ab436cab8f61cb646
- tmp/transport-4180-census-diff.local.json: 04470f08c38c9cfb4c04d74b2e5e0411b6c1859bde4dba7e7edc437ef0c7d549
- tmp/transport-4180-site-reanchor.local.log: 230578a1e1b78bbb9c12ef9f015f57cb2e559cd75c16b1364632343d9e992ff2
- tmp/transport-4180-contracts.local.log: fb3128ea6cea2359cce4eda435cdfdbf8961f55a3ef5722f96889e55da48e5d0
- tmp/transport-4180-writer-tests.local.log: 209ba17e6accd8d3845078a0d57f82041fef10cb7c7285a1788c72ba7ac10841
- tmp/transport-4180-journeys-driver.local.log: 76daa0a170a36547a196767068477e2f621ef2edad62d31eb9265d6f92b97fdf
- tmp/transport-4180-journey.local.log: 685f2e3e799d4c72c70fd3d8aff411e4805cb9b5549656e9cbcc7c9eba5c8607
- tmp/transport-4180-network-journey.local.log: 36115407587f0087540f001bdff7edee83ed8d0d83463c7448c25d48caca8732
- tmp/transport-4180-network-measurements.local.json: 8bb1f335a04058c0df5949f2c64c66657689d4241ff470ae0e9d49b3e2f44378
- tmp/transport-4180-recovery.local.log: efb878a1e87419c902e0c9cbc59cbef497ca3e33ff1be8f786595fc272d576e0


## Native SQL runner dependency and API repair

Exact strict compilation reproduced two maintained-test failures: the currency
runner's item link omitted current native dependencies/query wrapping, and the
legacy PA coin harness duplicated inherited pool discard and called an obsolete
accounting API. After fixing those, the PA runner also failed its current link
closure. The item and PA commands now use current accounting/coordinator/
player-recovery/quest dependencies; the null connection checks the actual
ENOTCONN error API. Existing guarded targets, cleanup, compiler warnings and
backend flags remain; the item driver matches its maintained 64 MiB stack.

The exact repaired commands compile with GCC 13/C++20 strict warnings:
item 113.250 seconds, PA 84.244 seconds, both zero diagnostics. Both Bash syntax
checks, seven typed-coin and ten currency contracts pass. All 1,229 native
source files match source4180/current bytes; manifest/tree hashes are retained.
Frozen binaries: item e50bef0ead8a0db43d8d1bfd067e79cb02a8e9d3d5334121bcafea724c633cbb;
PA 22547f9ac2f850bd61a1258fbf6489dd0f0684cc9541b8e9c63ce168ac35992b.

Primary native execution passes both complete item and legacy PA coin matrices
on actual disposable MySQL and MariaDB, with separate fresh family schemas,
the currency runner's current bootstrap/immutable migrations, coin-schema
corruption/upgrade checks and native verifiers. Docker is unavailable here;
services were freshly provisioned on loopback with private datadirs and
terminated after each engine. This is exact-binary native component execution,
not a Docker driver execution, player publication journey or production pool
lifecycle proof. Runtime replay/rollback/custody/craft/quest/quarantine checks
retain their bounded fixture scope. Production source and inactive behavior
are unchanged. The separate coin/item actual lost-COMMIT reply and ordinary
drop payload/publication requirements remain open; full R1-R8 stays blocked.

Runner milestone LF-normalized source/evidence SHA-256:

- tests/async/run_currency_transaction_schema_mysql.sh: f41a4d97f11555f924b41be4d1806ed20a84d2ddc14af124e6d8678b648604db
- tests/async/run_pa_coin_sql.sh: e3c53dd7876d2c305581c42218857f11da5c0b6e8852bdde7cbdd90360c48350
- tests/async/pa_coin_sql_harness.cpp: 4a1d57e224bcaebfcfe1b16df125693f92bb635090eef986d4a7592fb6178372
- tmp/embedded-item-red.local.log: af772f33a8e9614e93fac1977833cdd1f159ae9afcd901cc91a6fe19c3c42ef1
- tmp/embedded-item-green.local.log: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
- tmp/pa-coin-red.local.log: b56693b8b0d51fde43f4cdde9a50f24c2afa37f5e652a1376345d9630ed661b0
- tmp/pa-coin-green.local.log: 8daf31e1f7a093997dccd4e9e109cc964bf14f085d51a7312672a3198b35bead
- tmp/pa-coin-complete-green.local.log: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
- tmp/currency-contract-green.local.log: 8e58acf5241e640766d986b592bbcd84af60a37fd6e739d205a6a6dcba113201
- tmp/typed-coin-contract-green.local.log: 83166b089fbcb82fac63f22522fa6ae99bb5e46368cd35d756277fd463dd6346
- tmp/native-source-pin.local.json: 93030a45182074517c56c00b18edaadc1d8e7e178b23cc1b29856536f35bd129
- tmp/currency-runner-native.local.log: e4b24c0c08a3d5264e64765d37185a6dd3407b6d9f4e1cbdebe41007d52ad0db


## Native coin/item successful COMMIT reply-loss qualification

Source review established the missing proof: earlier coin/item coordinator
fixtures returned synthetic ambiguity only after repository success. The new
shared client wrapper lets the actual server COMMIT succeed, conceals its reply
and exposes error 2013 only for the selected pooled connection. It requires
exactly one hidden COMMIT and one replacement of that connection. The real
repository reconciles on a fresh session; the coordinator receives already_applied
without synthetic ambiguity/retry. Item's pre/post-COMMIT SIGKILL hooks remain.

Frozen source is 4180f74573c8a4cfe669c057c36f9c36dfc6d7fa: 1,229 archived files,
zero mismatches. Strict GCC 13/C++20 coin and item builds produce zero diagnostics.
Exact native binaries pass complete coin/item matrices on fresh disposable
MySQL and MariaDB services. Original UID/revisions, denominations, accounting
root/child/reference counts, same-ID changed-envelope refusal, retained journal
shutdown/init replay, identical result, held publication, one ACK/checkpoint and
empty second restart are checked. Actual engine instances and native binaries
were executed; Docker runners were not available. Binary SHA-256:

- coin: 7bc625930ba85b46bcee32514604b394883f0b790e23ecd14eefa0d0de65e2a0
- item: 9a7fc64d5d7426671db2af2b8b081ff8eee1263abe9a2a112f4f4be255c69460

Ten currency and seven typed-coin contracts, sanitized flatfile accounting
refusal, maintained Bash syntax and whitespace checks pass. The shared fixture also passes the actual maintained bank journey on both
engines under AddressSanitizer/UndefinedBehaviorSanitizer, including its native
COMMIT reply-loss and historical receipt/rejection/rollback matrix. The inherited
PA runner passes its exact current strict link with both query/error wrappers. The native pool
interfaces here open fresh connections and ACK through a fixture; production
pool lifecycle/contention, actual network loss and ordinary live publication
recovery remain open. SQL room-payload implementation is separate WIP. Full
R1-R8, captured-clone, complete coverage and measured accounting workloads
remain open; accounting stays inactive and release BLOCKED.

Reply-loss qualification LF-normalized evidence SHA-256:

- tmp/real-commit-reply-build.local.log: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
- tmp/real-commit-reply-native.local.log: fab71b2c9064d65d6b2d5713deff04006fd1cca80f5de829721bf09d3a4e90fe
- tmp/real-commit-reply-bank.local.log: 0b00193f6d95e02dead1c3f1b3ed1f3681fc939beeaede93746061655d6f1ab4
- tmp/real-commit-reply-pa-build-current.local.log: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
- tmp/real-commit-reply-contracts-final.local.log: 45ef35aa4a90167bad88d868d7066f34a772d91922604168093addd23056f402
- tmp/real-commit-reply-source-pin.local.log: a823ec37b37a89b6249b2db89641522c450a6709cffbf2785a28659b0c3131c3


## SQL replacement consumes its borrowed lease on every outcome

The native one-slot shutdown regression fails before this fix: replacement
returns NULL while sql_pool_in_use remains one. The critical repository drops
the old pointer and cannot release shutdown. Other NULL paths previously closed
and freed that pointer, so retaining/releasing it in callers was also unsafe
once an address could be reused by a later borrower. Normal shutdown's usual
coordinator drain ordering does not waive the violated lease contract.

For a valid owned lease, replacement now returns one fresh borrowed handle or
NULL after retiring/releasing the original. It preserves the reservation while
opening outside the mutex, catches factory exceptions, handles discard/closing,
and closes any unpublished fresh session before shutdown can return. Invalid
foreign/unborrowed handles do not modify a lease. Snapshot, death-conflict and
locker callers assign the returned pointer, including NULL, before any release.
They retain ambiguous outcomes; the fix never infers a failed/committed debit.

Strict and ASan/UBSan pool tests pass actual mutex/deadline behavior, closing
before and during factory open, failed/thrown factories, discarded input,
healthy-peer availability, clean successful replacement and unborrowed refusal.
Both native SQL engines pass sanitized real-session retirement/rollback,
subsequent commits, clean capacity recovery and closing-replacement shutdown.
Worker timeout/gating/init, locker repair and critical transaction contracts
also pass. Production SQL/flatfile strict builds, up-to-date pfile dependency
check, formatting and fresh inspector build (118.121 seconds) pass. All ten
native recovery cases pass in 425.720 seconds, including both engine dump/import
and private boot, pending replay, WAL/quarantine and locker/spell receipts.

Qualified native source tree c60330b58de9063dc1ad8510ced36310324d5d4c matches
all 1,229 archived files. The concurrent room-payload WIP is excluded from this
source and is not qualified by these results. Executable SHA-256:

- SQL: d4cd5061795a6450249df075292d6ac94a851e90d5d3bd897bee207a26c3685c
- flatfile: a2c4b43165a715184a9ccad4d2fab93836a2bc3af916177f02627dc5def123f9

The first recovery driver omitted /usr/local/bin from PATH and stopped before
testing; that failed setup log is retained. The corrected driver reuses the
same native binaries and freshly compiled inspector. Actual typed coordinator
COMMIT-loss with this pool, production configured-factory contention and live
publication remain separate gates. The exact published-70aaa broad suite is
still running; full R1-R8/captured-clone/workload/coverage completion is open.
No production accounting or inactive behavior was activated.

Pool lease qualification LF-normalized evidence SHA-256:

- tmp/pool-consumed-lease-red.local.log: 32ec51ff80e29cc956d14863d2e70727390e395103fd517f468c94bfd6c2d36d
- tmp/pool-consumed-lease-green.local.log: c265a737b7742d164f397e08c8c368cadc383ed1e493427231df5a8c5a388f33
- tmp/pool-consumed-lease-final.local.log: d561607e2047afa6b0f1bf52d8cefd81be5f45111cab563e604633b247b1975e
- tmp/pool-consumed-lease-native.local.log: 2538d999b77c27d17ce109f4352370f29ebc4271b46e9f9e9b56641328c3aaec
- tmp/pool-consumed-lease-contracts.local.log: a2264d8ac723b34b802627b63188ec8909ad50fc959863ca07c5b55d827fe34d
- tmp/pool-consumed-lease-build.local.log: a651cbe981d14d23bc96490c69559efebca83614084ab2b30b13a9b850f517a9
- tmp/pool-consumed-lease-format.local.log: 4c6455b35b80e3160133eea6574bea1703c3e598f93d3f345baa3a1925e88614
- tmp/pool-consumed-lease-source-pin.local.log: 54fad6a012951ec347fb13b375a8c57587a52841e1961c3ea768b2292f450257
- tmp/pool-consumed-lease-recovery.local.log: 78021c998c37ac09818e4bf25e8df8910e7492fcb87871d6fc0c858c9f8bcb3f
- tmp/pool-consumed-lease-recovery-final.local.log: b08258f2a93b77b9c917ac331721f53957454e1e19cd9c170da72df5ed38c077

## Actual production-pool coin/item coordinator qualification

The earlier native coordinator matrices supplied pool interfaces. This bounded
fixture now links the actual sql_pool.c implementation. Wrappers only observe
acquisition/release/replacement and arm the real successful-COMMIT reply-loss
fault; the actual pool owns leases and replacement. Its disposable configured
connection factory is supplied by the fixture, not production boot.

Strict GCC 13 builds with ASan/UBSan pass both complete coin and item matrices
on fresh private MySQL and MariaDB schemas. Lost successful COMMIT replies
require a different server session, exact once-only replay/result, unchanged
UID/revision/value witnesses and held publication until fixture acknowledgement.
Every run finishes with zero borrowed leases, a clean autocommit reborrow, one
available slot and completed shutdown. Nine unsafe runner targets refuse before
compilation/service access. No Docker execution is claimed.

The frozen native source c60330b58de9063dc1ad8510ced36310324d5d4c matches all
1,229 archived native files, with zero mismatches. The room-payload WIP is
excluded. Test-only additions are frozen at
/opt/duris-accounting-real-pool-coordinator-review/source. Binary SHA-256:

- coin: 1205074caf2ed169e2237890884637f60bafe18bf57e06c08ecaab1ca1133df4
- item: 47d357313744783b3dc5d13fa541a2a899389ff83e03554167f62acde90757ee

The typed bank owner still requires this actual-pool qualification. Production
factory/boot, mixed contention, actual network loss and ordinary gameplay/ACK
cold-boot publication are separate open gates. This component milestone does
not complete R1-R8, captured-clone or measured accounting workloads. Accounting
remains inactive and release BLOCKED.

Evidence SHA-256 (LF-normalized):

- tmp/real-pool-coordinator-build.local.log: c071af53c25f92bfe0b1d61bb5d63af4179d7c7e2c590bdcc4443ba09a7c438a
- tmp/real-pool-coordinator-native.local.log: 72427ba2f53d1d7d231dd4c22cf67972ca0b7db68b38ca896b5bdcff22e61fcc

## Six broad-fixture dependency/lifetime repairs

The frozen published-70aaa broad run established compile failures after runtime
identity and dispatcher wakeup integration. Five retained fixtures lacked the
actual authoritative identity-map linkage/lifecycle; the dispatcher lacked its
wakeup dependency. Fixtures now register and retire identities through the real
production map, preserve delayed-completion assertions and test retired identity
refusal after storage reuse. The mock-layout spell fixture extracts the actual
map implementation. Dispatcher uses the real header-local pipe and checks
readable notification and drain. No production spell path was changed.

All six focused owners pass strict native warnings and ASan/UBSan: corpse's 15
batch combinations, input queue, publication retention/fences/allocation cases,
spell publication, dispatcher's three quarantine/retry cases and zone-purge's
four immediate-free/move combinations. The corpse secondary wakeup harness
retains its existing unsanitized build. Source dependencies are pinned at
8cc56007b8fab2dd7bfdbad2c52ac338c055478c. Primary verified all six current owner
and clean log hashes against the native evidence. Shared helper formatting and
whitespace checks pass. Initial failed/mixed logs are retained separately.

The qualification is component-only, with synthetic journals/apply boundaries
and spell mock layouts; no database/live-player qualification is claimed. The
original frozen broad suite continues with its original failures. These repairs
are not a passing rerun or full R1-R8 completion. Accounting remains inactive.

Evidence: tmp/fixture-identity-six-summary.local.json SHA-256
31788b08bf13524d79909d3df24c5b11967162dad70136422c02832092a3e669.

Remote 44f4c54035ad54e0b3450e14346c8150be4ff2f6 integrated PR 693 while this
milestone qualified. The fixture commit rebases onto that update. Its new
always-null retention lookup stub is removed in favor of the actual linked map.
All six focused owners pass again with incoming native alchemy/save-ACK changes;
44 key source/header/fixture inputs remain unchanged through qualification.
The first concurrent corpse compile exceeded its unchanged 180-second gate
without diagnostics; the serial retry passes that same gate. Both logs remain.
Primary verified all six current owner/clean-log hashes. This integrated
component evidence is separate from the earlier 8cc56007 qualification and the
still-running frozen broad suite. Integrated evidence:
tmp/fixture-alchemy-six-summary.local.json SHA-256
5c38979f6d840a29ea06c640d0ee469a08d0366d0fae83423ef428176c6775ac.

## Regression resource-owner inventory repair

The exact regression-runner contract omitted two integrated transport journeys,
reproducing a RED inventory assertion. It now contains all 18 expensive owners
and proves they partition serially in order. All seven existing runner behavior
checks and Makefile/discovery/manual-owner checks pass. These are synthetic
runner checks, not native gameplay or a passing broad rerun. Primary verified
owner, dependency and RED/GREEN log hashes against
`tmp/contract-drift-two-summary.local.json` SHA-256
286346cc1531dcf2cd9ce04cb9b217c5458e9cc36c4708f5b5dd866fa30855de.

## Integer deadline timing contract repair

The timing contract still expected the removed timeval sleep budget. It now
checks the actual monotonic integer wait/deadline/timeout helpers, clock-failure
shutdown, expiry-before-poll, bounded integer timeout and loop call. Twelve
source checks and five negative mutations pass; deliberate missing/reordered
guards remain RED. Owner/dependency and RED/GREEN hashes were verified against
the same contract-drift-two evidence above. This is static contract evidence,
separate from previously measured native listener/cadence budgets and the
unfinished accounting workload and broad-suite gates. No production code changed.

## Frozen transport-integrated broad regression final result

The unchanged frozen 70aaa41384cb40ae786d06241ec5313ccafef822 run finished:
831 passed, 11 skipped and 17 failed across 859 owners in 7,988.520 seconds.
Results remain failed; later component fixes do not relabel this run. Actual
account recovery, persistent transport, network readiness, player quarantine
and static quest journeys passed within that scope. Original failed owners
and all original logs remain in tmp/qualified-70aaa4138-results.local.json and
tmp/qualified-70aaa4138-broad.local.log. Current-head broad qualification,
flatfile combat/first-session investigation and writer-census refresh remain open.

## Deferred item-output native fixture dependency repair

The maintained prompt fixture reproduced unresolved native runtime lookup and
TLS direction dependencies. It now links the production runtime identity map,
registers and retires actor/body identities, checks retired-ID and worker refusal,
and supplies an abort boundary for unexpected TLS use. The unchanged deferred
item/currency, ambient bytes, auxiliary prompt and switched-descriptor parity
matrix passes under ASan/UBSan with the existing compile/runtime time gates.
The added TLS boundary supplies linkage; no native TLS handshake is claimed.
Native direct dependencies equal published e36644777a9a0c88a631317e590394d2f1582163.
This is component evidence, not live player/database publication or a broad pass.
Evidence: tmp/prompt-dependencies-summary.local.json SHA-256
45eddac2d2943c4f72aa3e3d3a7d0656a1bf038faa4771f9c51da59cb30dd310.

## Descriptor poll and connection-capacity contract repair

The old contract still required FD_SETSIZE rejection after readiness moved to
dynamic poll registration. It now checks the actual registration/readiness and
live-capacity admission boundaries; eight unsafe source mutations remain RED.
The existing native readiness harness also passes under strict ASan/UBSan with
an actual descriptor above FD_SETSIZE. This supplies component readiness proof,
not listener load or complete gameplay qualification. Primary verified the
owner, manifest and all five frozen qualification log hashes against
`tmp/contract-drift-three-summary.local.json` SHA-256
8afeaec1177fcf411ee513dd806ffa5ad6bdf76bd54113f23b7e0e0e543a687d.
The original frozen broad failure remains recorded separately.

## Mail-worker wakeup-leaf boundary contract repair

The mail-worker contract incorrectly treated the POSIX-only network wakeup
leaf as an engine dependency. The revised boundary verifies its complete
header closure and rejects transitive engine headers, pointers and callbacks.
All 653 account source checks pass, including four negative mutations; actual
mail_sender.c also compiles standalone with strict warnings and no engine link.
The frozen three-contract evidence above pins this component. A separate final
current-source scan retains all 1,228 production source hashes through the run:
`tmp/contract-drift-account-current-final-summary.local.json` SHA-256
edc29be0862760139976abf3052f73450b944a6aa40c3bb60af0b60a4a0cd748.
No SMTP/service or full accounting qualification is claimed.

## Kingdom removal-generation ordering contract repair

The cache invalidation contract omitted the new runtime-identity retirement
before extraction. It now requires retirement and one generation increment
immediately after the null guard, before maintenance or nested callbacks.
All 441 kingdom source checks pass, including six mutations proving that
missing/reordered invalidation and retirement still refuse. Primary verified
the owner and frozen log in the same three-contract evidence above. This is
source-contract evidence, not a new native combat/kingdom gameplay proof or
passing broad rerun. No production source changed for these three repairs.

## Native SQL fixture formatting gate repair

The published real-pool currency and item fixtures failed the maintained
clang-format gate at three whitespace-only hunks. Both owners now match
clang-format 18. The native candidate QA check passes for all 1,494 tracked
C/C++ files; its copied session queue fixture mode was restored to the
published 100644 before the check. No production behavior changes.
Evidence: `tmp/real-pool-harness-format-summary.local.json` SHA-256
3594de85a8b8faea5e3c02fe774b727b3a71f9d18742ce0d55fddab8e28274e2. This formatting result does not replace native gameplay or
accounting acceptance checks.

## First-session native inspector build-budget repair

The first-session entrypoint retained a 180-second compile deadline after the
shared native inspector acquired the maintained 600-second build budget. It
now uses that same build owner. Gameplay deadlines remain unchanged. The
current candidate flatfile binary passes empty/populated bank first-session
credit, retry, save and reload, plus all three maintained combat/death/corpse
recovery/save/reconnect cases. Inspector compilation took 62.972 seconds in
this run. Evidence: `tmp/first-session-inspector-budget-summary.local.json`
SHA-256 70f2c2a3f08dddd3858557bf6a61ec377acbf64bd6e991b959a0f8043336e26f. The frozen 70aaa failures remain
recorded; this is current component/gameplay evidence, not active-accounting
qualification or a passing current-head broad suite.


## Exact historical SQL room payload and cold recovery

The previous durable custody graph lacked a complete native room payload after
player projection retirement. Migration 0055 adds an immutable, bounded payload
sidecar keyed by original UID and item revision, with operation and season
provenance. It does not introduce another custody authority. An already admitted
schema-2 ordinary player drop with one complete full-literal root graph records
this payload and retires its player projection in the same transaction. Missing,
stale, corrupt or unsupported evidence refuses; inactive behavior is unchanged.
Native cold restoration preserves UID, topology and every canonical payload byte
without replaying live room physics or minting a replacement graph.

Both disposable MySQL 8.0.46 and MariaDB 10.11.14 pass the actual production-pool
ASan/UBSan rollback, projection-delete fault, real lost-COMMIT-reply replacement,
exact reconciliation and retained ACK matrix. Both also pass two independent
complete-world cold boots without Redis, with native payload and custody readback
and unchanged SQL history. The seeded fixture uses native writers and a coherent
three-node literal graph; it is not an actual player-issued drop or active cutover.
Generic unstrung producers, held gameplay publication and retained-drop replay
remain open and retain their refusal gates.

Canonical and upgrade-prefix migration histories converge at 55 receipts and
226 tables on both engines. Original staging/master prefixes remain unchanged;
appends are ten and 24 respectively. These are disposable bootstrap-derived
forks, not captured staging-clone qualification. Both maintained strict native
production builds, all ten native restore cases, migration/lifecycle/boot
contracts, payload/publication ASan/UBSan owners and new-source formatting pass.
The pfile target remains up to date. The qualified native QA matches all 1,229
public native C/C++/Makefile source entries after explicit LF normalization.

Evidence: `tmp/room-payload55-final-qualification-summary.local.json`, SHA-256
4c2d9461a77dc542afcc55d89818d6bba34f4683cbbc46ddb1007817e52204ad,
pins source manifest, native binaries, seed, driver, logs and both cold-boot
readbacks. SQL production binary SHA-256
87bb4bd1aeff3662011acb3e65869fc0b526a303916c2d8d76b372764ac9118d;
flatfile production binary SHA-256
f0c37cd6d62caef1a5481ba90b28c74592cd5a4206f10f473481fc4ff2414964.
The separate observer binary supplies exact native readback only under a test
macro; production does not enable it. The original frozen broad result remains
831 passed, 11 skipped and 17 failed; a current-head broad run and full R1–R8
qualification remain required. Release and activation remain blocked.


## Published room recovery writer-census repair

The lexical scanner and owning registry now include immutable room payload
insertion, exact hydration, exact placement and detached stage cleanup. Reviewed
source anchors are refreshed without promoting component proof to executable
backend coverage. The matrix is pinned to published native source
5f542e9cf769dfa7ef89e28ce4845d7495d4e1b3, with no unpublished candidate claim.
It records 868 routes, 2,817 occurrences and 2,758 unique sites, zero unmapped.
All 54 coverage tests, three issue-590 census tests, 2,727 writer-site checks,
14 contract fixtures and reproducible generator check pass. Release validation
still refuses missing executable evidence as required. Native recovery evidence
does not qualify the real producer or full writer inventory.
Evidence: `tmp/room-payload55-census-final-summary.local.json`, SHA-256
de9d84286097c91951b62d2c34a2316331ac537abb47f9199228911b248adeb1.

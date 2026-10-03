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

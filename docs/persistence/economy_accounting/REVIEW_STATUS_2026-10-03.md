# Experimental accounting review status - 2026-10-03

The requested scope remains full R1-R8 implementation and qualification;
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
831 passed, 11 skipped and 17 failed; a current-head broad run and full R1ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦ÃƒÂ¢Ã¢â€šÂ¬Ã…â€œR8
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


## Typed bank production-pool qualification

The bank owner previously supplied fresh fixture connections in place of the
production pool. An opt-in guarded real-pool mode now links sql_pool.c while
keeping the complete existing bank matrix. Observers forward pool calls and
hide a successful COMMIT reply at the native client boundary; reconciliation
must borrow a different server session. Both disposable engines pass writes,
exact retained replay, authority retirement, forged witness/code and native-state
refusals, rollback faults, retained tamper checks, clean autocommit reborrow,
zero borrowed leases and shutdown before mysql_library_end under ASan/UBSan.
The guarded connection factory disables reconnect and forwards native flags.

Primary verified 581 linked-source/compile-owner/native-header entries against
the frozen build after explicit LF normalization. Native binary SHA-256:
fa2745f384202e22eba17abcdd79aec07d5bc1fb9711e63714d4dc9cdf4bff3c.
Evidence: `tmp/bank-real-pool-payload55-final-summary.local.json`, SHA-256
5f6a826d033f5d658d781fb0fd5f9496c898dcfae4171994b80e8f76214d1513.
Formatting and eight disposable-target refusal mutations also pass. This closes
the bank actual-pool component gap; production factory configuration, mixed
workload contention, actual player publication/replay and current-head broad
qualification remain independent gates. Activation and release remain blocked.


## Selected native full-literal capture owner

Native ordinary snapshot capture preserves prototype-relative masks and omits
unstrung literal strings, so it cannot supply the exact stored room payload.
Two opt-in adapters now freeze all four effective strings for one selected
carried root and its complete descendants without changing live masks. Other
inventory roots, equipment, pets, ordinary captures and formats retain their
existing behavior. Missing/noninventory roots, duplicate/zero or reserved-sentinel UIDs,
cycles and all native capture limits refuse atomically. This does not establish
UID allocation authority; the eventual transfer still proves native custody.

The actual capture/codec/custody fixture first reproduced missing API linkage.
It now passes strict ASan/UBSan, direct native-field assertions, byte identity,
mask0/partial-string capture, nested topology, scope isolation, no-rent graph
retention and unchanged failure outputs. Existing native capture/death/held-pet
and item-codec regressions pass. Both maintained strict native server builds pass
in a source-isolated published-base QA containing only this capture issue.
Evidence: `tmp/literal-capture-native-summary.local.json`, SHA-256 aef2b981f08d957942ac170499c70048afd41858d6e3b3e136abd3ae36d24466.
SQL binary SHA-256 33e5b1dfa57aeff4d9c5385a4bf3007a86f92c9160b9516a0a79577be6c2a771;
flatfile binary SHA-256 6b43838647574d9bfbab526b9a523243bba470c8df001b25d149a654adc416a5.
The initial final-run invocation used a nonexistent codec owner name; its original
log is retained, and the correct maintained item-codec owner passes separately.
Scoped checkpoint coalescing, actual SQL source proof, retained drop publication
and replay remain unfinished. These APIs do not enable inactive accounting or
establish full gameplay/persistence/recovery qualification. The frozen e58bb3296
broad run finished with its original failures and does not contain this
subsequent capture change. The final frozen result and follow-ups appear below.

## Exact physical properties required for SQL room source proof

The actual native pool fixture reproduced a source-proof defect: setting the
selected player's physical `item_properties` projection to NULL still allowed
`sql_room_item_payload_prepare` to accept the captured dynamic-effect payload.
The native snapshot writer emits canonical properties bytes; NULL cannot witness
that write. Preparation now requires exact non-NULL binary equality and refuses
this missing projection with ESTALE before the item operation mutates authority.
The existing inactive command path and accounting admission gates are unchanged.

Fresh disposable MariaDB and MySQL pass the actual production-pool component,
including the new NULL refusal, exact source/provenance/season checks, both
partial-operation rollback faults, real successful COMMIT reply loss and distinct
replacement connection, exact-ID reconciliation, ACK, and two independent cold
SQL reads. The original assertion failure is preserved separately. Both strict
production SQL and flatfile builds pass; the native payload codec/capture test
and 54 coverage contracts, 2,727 writer-site checks, 14 accounting contracts and
generated-matrix check pass. Identical INSERT anchors moved from line 588 to 586;
backend route qualification has not been upgraded.

Evidence: `tmp/room-source-properties-final-summary.local.json`, SHA-256
9f73753b37f5ad9d04af8ac6ab875d4a80f2fe907e5db364c008d84aada92e90;
587 LF-normalized linked/compile-owner/header inputs match the source-isolated
production candidate. Native pool binary:
0d4cb81d35d71fdb949bf1d0f9bc3d45a9397d9a9a46d38eb45e238f4380f421.
Production SQL: 41bd0dd301f1e594d58adb46d84b6093bef135b9d909fa501051328501e2f72c;
flatfile: 5a84db2dc851c73e80bcd05345529521ed18e7537440df0bda3aedd47987ecee.
This closes one physical source-refusal defect, not actual player-drop producer,
held publication/replay, current-head broad regression or full R1-R8 completion.

## Scoped literal checkpoint and lifecycle ownership

A generic ordinary recapture could replace a selected full-literal graph with a
prototype-relative graph, and a local journal acknowledgment could be mistaken
for the database source checkpoint. The new opt-in PID/runtime/root/generation
scope retains its literal policy through newer ordinary captures and coalescing.
It reports database acknowledgment only for the exact successful captured worker
revision, with complete selected bytes and a clean revision/queue state. Fresh
native graph mismatch or authoritative actor-map retirement refuses the token.

A fresh quiesced scope can bind to one original operation ID. The hold blocks
capture and terminal/death/target-login admission before revision or queue side
effects; ordinary dirty marks continue advancing. Pre-admission cancellation
cannot erase a held operation. Only its original operation ID releases it.
Unheld scopes retire on stale identity, changed intent or terminal supersession;
late completions cannot acknowledge a replacement scope. No gameplay caller uses
this API yet. The eventual native transfer must still prove the actual physical
source inside its atomic transaction; this API's database ACK is not that proof.

Fresh disposable MySQL and MariaDB pass actual production capture, pipeline,
journal, worker, repository and pool integration under ASan/UBSan: paused older
SQL apply, newer coalescing/recapture, exact ACK, complete literal SQL payload and
custody, real successful COMMIT reply loss, original-operation hold ownership,
held terminal/death/login refusal, unrelated PID admission, immediate unregistered
actor refusal, late old-runtime completion, changed intent cancellation, and two
independent cold component reads with positive native journal replay/no quarantine.
These are inactive component fixtures, not actual player-drop or full-world
accounting journeys. Twelve disposable-target guard negatives and strict compile
within 600 seconds pass. Both strict production backend builds pass.

Flatfile begin/poll/hold refuse before capture, token mutation or save admission;
a local flatfile journal completion cannot be labelled a database acknowledgment.
The SQL-only generation declaration retains strict flatfile warning checks.

Evidence: `tmp/literal-checkpoint-sql-only-native-final-summary.local.json`, SHA-256
7bff5062f3840181754eb78b3995b8b3f43a8805a0a811b90c076cd789dc3c44;
537 linked/runner/header inputs and 1,229 strict production source/Makefile inputs
match the current worktree with explicitly LF-normalized hashes. Native raw
compile hashes are retained separately. Actual native component binary:
f0af34f9ba852f36dba337de0e4f45d5478cad99bdac3aaf085b1df1649cd676.
Production SQL: 55145a53215d15a9e5fcb1e303e953535a850d057dfdd42aa374e99adf364970;
flatfile: 01428fda7e29b8fa67e0db1a23f42f81b5bc3467f7347b6e886052e15d7d7c17.
The final strict native compile passes in 270.943 seconds within the unchanged
600-second budget. The save, stable death, public death and creation owners pass
14/4/9/24 groups against final source; the stable extraction now ends at the
actual array declaration rather than retaining an unmatched preprocessor guard.
Their final artifact is
`tmp/literal-checkpoint-sql-only-final-declaration-artifact.local.json`, SHA-256
ef7a287e2680ceb92b18f686796ec64e205045fedee458c82cf4b01c2f553b7f.
Eight earlier assertion-rejected native guard mutations and their exact helper
body equivalence remain separate evidence. The separate
ordinary-drop preparation, physical handler/action preflight, retained boolean
publication, publication-ACK release and restored save/lifecycle obligation
remain open, with implementation order recorded in Plan 1. Full R1-R8 and
current-head integrated qualification remain open; accounting stays inactive.


## Current room migration fixture owners

Published f787dc9be refreshes the exact inventory owners to 226 tables and
0055, preserving the room payload module in the supplemental SQL source set.
The three focused owners, runtime compatibility, lifecycle registration and
immutable migration histories pass. Evidence:
`tmp/room55-inventory-owner-summary.local.json`, SHA-256
00920885b2703b76d979961b294a9c995f1a780789bae4699f5358c18056c79f.

Published 5f83fbc78 links the real exact room payload owner into the corpse,
item-transfer and direct SQL gate native fixtures. Strict ASan/UBSan corpse and
item owners pass in 210.378/137.428 seconds; actual client-linked SQL gate
assertions pass. The gate still refuses closed SQL access, admits typed bank
initialization and rejects null root connections. Evidence:
`tmp/room-native-link-owner-qualified-summary.local.json`, SHA-256
6feb53a93439dd5a608d96519e4dfdbe33eb0ea57e6fb0a70091b9821645a88f;
571 current LF-normalized linked/compile-owner/header inputs verified. These
repair frozen broad fixture failures; they do not relabel the original run.

## Creation journey failure evidence

Published 009085e12 captures bounded assertion, socket and shutdown-timeout
failure evidence before isolated runtime cleanup. Client connection is inside
the capture boundary. Password/email/environment secrets and bcrypt hashes are
redacted before per-file or final tail limits. Default recovery log collection
and the existing five-second prompt wait remain unchanged.

Three controlled failures pass through real minimal isolated flatfile server
boots, retaining native output/runtime witnesses after the temporary files have
been removed. The frozen e58bb3296 binary is
be18949354c85c25672e34189aff807f0c079fdc0934eac0bba8f85d46eb4a6c;
the diagnostic overlay and scope are pinned in
`tmp/creation-native-diagnostics-summary.local.json`, SHA-256
d9542ded2934505ee25dcaa77d93ad2c4c013494e0e09d095fc8921f5519e130.
This proves failure diagnostics, not starter-kit completion or current-head
player gameplay. Initial boot failures retain their separate existing cleanup
path. The frozen broad run's invitation-labelled failure was in its initial
human warrior STAFF starter-kit wait, before restart/invitation/evil selection.
Its 600-second supervised server-build and 180-second inspector timeouts remain
recorded in the failed broad result. Serial follow-ups below retain the original
budgets. The frozen broad run finished at e58bb3296 and excludes subsequent
source fixes; final current-head integrated qualification remains open.


## Read-only pending item-action custody preflight

Ordinary-drop preparation lacked a read-only way to refuse a selected source or
adapter-referenced descendant. Its departure hook can run resource-changing
finish handlers after `obj_from_char` has removed the carrying-list link. The
new `item_actions_object_busy(uid)` checks all pending source and typed adapter
references without cancelling, finishing or changing resources. Unknown IDs,
off-thread calls and incomplete pending metadata refuse before unsafe access.
No gameplay caller has been wired.

The complete production scheduler/item-action owner passes under ASan/UBSan,
including its original identities/costs/timing/cleanup tests and three new query
groups. Source and adapter references, later pending entries, actual completion
and cancellation, unchanged reservations/resources/events/entry state, and worker
thread refusal pass. ASan/UBSan and ordering checks are not race-detector proof.
Six query guard/reference/iteration/mutation negatives are assertion-rejected.
Both strict production SQL and flatfile builds pass with their original flags.
The native owner retains its original flags, passes 15 groups and compiles in
35.713 seconds within the unchanged 600-second qualification budget. Its
64 actual compile/extract/static dependency pins match the worktree. Specialist
artifact `tmp/item-actions-busy-qualified-artifact.local.json` has SHA-256
e4df4d2ea9890c8956720e00d8a55ee88688f17f333a647f588e2852c8701f82.

Primary evidence: `tmp/item-action-reference-qualified-summary.local.json`, SHA-256
e8adda45fce6b0dca6d142402464f3a39801af964ef8877b3a24e868ef65a6d8;
529 current LF-normalized owner/source/hook/header inputs and 1,229 strict
production source/Makefile inputs verified. Native component binary:
8c594336059ac67633fedde3282afa451923bfa47b927bafbf5e49e5e2adc6e8.
Production SQL: e60eb3f7e1041207f585ea964002b79e7a52eb531e6ffcc7f8e390e563e6cf2d;
flatfile: 4cc3d9414a53f9b42ac19f7aa279ea9ea94ced080b7a939073aaf02eee1d6f1f.
This qualifies the preflight API only; actual producer, physical handler refusal,
held publication/ACK, restored lifecycle/replay and current-head gameplay remain
open. Plan 1 records the distinct authoritative-hydration design requirement.

The original invitation-labelled creation failure scenario now passes in
32.920 seconds against the verified frozen e58bb3296 server with the published
diagnostic-only fixture overlay and unchanged five-second waits. Reproduction
log `tmp/creation-starter-kit-qualified-reproduction.local.log` has SHA-256
cd8a6d1fb1988a09647f6eff8b86f6d2e6787c2ac74ca48d473c92d941eb23d0.
The first repeat passed the initial kit wait but lacked the native linker wrapper
for its later helper build; that environmental failure remains preserved.
One passing repeat does not explain the original intermittent creation/network
starter-kit timeout or qualify current-head gameplay. Those gates remain open.


## Retain dependent durability owners after refused critical shutdown

Final app teardown called player-save and locker-async shutdown even when the
critical coordinator refused to stop. Player-save shutdown clears operation
holds, so that exceptional branch could discard a still-owned obligation.
The dependent branch now requires successful coordinator shutdown and retains
the existing pwipe exclusion. Successful/inactive teardown order is unchanged.

Five original source contracts pass before the fix; the added native actual
conditional-fragment assertion fails on coordinator refusal. After the guard,
all six tests pass with strict warnings and ASan/UBSan, covering the complete
coordinator-result/pwipe matrix. Its leaf shutdown functions are controlled
counters/retained-state doubles: this is not a linked refused-coordinator/save-
pipeline integration proof. The exact production fragment changes only the
branch condition. Specialist evidence:
`tmp/critical-shutdown-boundary-artifact.local.json`, SHA-256
3e11b4f7b80dd72538a2de096208a7cc97ebd2226d6e0ac05a878d4eb4d8c9eb.

Both strict production backends pass, and the current flatfile binary boots and
shuts down normally in a real minimal isolated world. Primary evidence:
`tmp/critical-shutdown-retention-qualified-summary.local.json`, SHA-256
86156c86ea621aac84c821cef294d58d01c1af0e3eb43c5d30fd5912408ab204;
four native/source-contract inputs and 1,229 current LF-normalized production
source/Makefile inputs verified. Production SQL:
1a337fe6339ec79febc1ad40a217b24457bb7b6a5d5b48712b43724915ca9c54;
flatfile: a82cad6ef84f8872fb5c8fb499534de2f6402335b17a273c14c6798512002af4.
The original isolated build-driver cache-path failure is preserved separately;
it did not compile or change production source. Restored drop registration,
authoritative hydration and original-operation ACK ownership remain unfinished.

## Final frozen e58bb3296 broad result and follow-ups

The unmodified frozen run finishes in 8,433.636 seconds: **840 PASS, 11 SKIP,
10 FAIL**, not interrupted. Report
`tmp/qualified-e58bb3296-results.local.json`, SHA-256
7d857ee576195e3d2398b6d9121787e4a896998594bbaf98ca3a5ea31f0753fd.
Six schema/link-owner failures have subsequent published focused fixes in
f787dc9be/5f83fbc78. Account recovery originally timed out in supervised server
compilation; area coin pickup timed out in its 180-second inspector compile.
Creation and network readiness failed the existing five-second starter-kit wait.
The original failed report is retained unchanged.

Serial follow-ups use the verified existing frozen binary and unchanged owner
runtime/budget assertions. Account recovery passes enabled/disabled mail and
cold relog in 17.407 seconds after acquiring the already qualified artifact;
this does not relabel its earlier cold-build failure. All three real area pickup
commands (`get coins statue`, `get all.coins statue`, `take all statue`) pass
wallet/native authority assertions with the original inspector budget. The
creation scenario repeat above passes with a diagnostic-only overlay. Network
readiness passes its existing capacity, queue spacing, latency and idle-CPU
budgets in 26.257 seconds. These are frozen-source follow-ups, not current-head
qualification or proof that the intermittent starter-kit failure is resolved.

Follow-up log SHA-256 pins: account recovery
ca48c22d8aac83dcc59bd701ae67e2f465c4234084de59821ca3e2e5f6ad5726;
area pickup 2bf1cc8cab9d8d4fbbf2c0a84dc3d52ec925927c4b2069657fe1de93017d6985;
network readiness d6291769c6d851f04bfb838384c575cac6168d5b731e5768371d16e691b1e083.
All R1-R8 release gates and the final current-head integrated run remain open.


## Restored SQL ordinary-drop obligation and authoritative hydration

The save pipeline now admits a bounded restored ordinary-drop obligation from
its original retained schema-2 command, without inventing a live runtime token
or capture/ACK revision. Registration requires an initialized, non-stopping SQL
pipeline, actual accounting-envelope support and complete ordinary literal graph
capture; save-journal replay need not have finished. Exact retries retain one
hold. Changed operation IDs/bytes, unsupported shapes, allocation failure and
shared 256-scope/32-MiB limits refuse without damaging prior holds. Flatfile
registration remains unsupported.

Account/nanny authoritative hydration now distinguishes restored obligations
from live checkpoint holds while preserving real quarantine, target, pinned-death
and degraded-recovery gates. Saves and lifecycle mutation remain held. Live token
release cannot discard a restored obligation; the original-operation release API
is allocation-free and actor independent. Its caller must already have a durable
coordinator publication ACK. No production replay observer or ACK caller uses
these new APIs yet, so this is component implementation and qualification only.

Seven focused owners pass. The 24-production-source native fixture passes strict
warnings and ASan/UBSan compilation in 458.533 seconds within its unchanged
600-second budget, then passes on fresh guarded MariaDB and MySQL targets with
actual capture, pipeline, journal, worker, repository and pool integration.
Existing exact-save/coalescing, physical SQL payload, actual COMMIT-reply-loss,
cold component reads and save-journal replay assertions remain. Added manual
restored-command checks cover replay-in-progress registration, runtime-zero
survival, clean hydration versus real fences, lifecycle refusal, exact/conflicting
retry, shared count/byte budgets, allocation failure, actorless release and
resumed saving. The pinned-death check is an extracted owner test; these checks
do not drive critical replay, publication ACK, copyover or an active drop journey.

Both strict production backends complete incrementally with unchanged flags and
-j2. Initial SQL and flatfile header-fanout rebuilds each exceed the original
600-second driver budget; their failed logs remain preserved and are not cold
build passes. The current strict flatfile binary also passes all actual inactive
creation, starter-kit, invitation/helper, hardcore, world-entry, save, cold-restart
and relog scenarios in 162.558 seconds with unchanged five-second waits. These
isolated worlds qualify inactive creation behavior, not SQL held-drop gameplay.
The initial native fixture omitted publication_required and failed its command
support assertion; its successful compile and failed MariaDB run remain separate.
The corrected factory has a native RED-to-GREEN reproduction. A focused-owner QA
run missing the public environment template is also retained separately from its
corrected passing run. No production admission gate was loosened.

Primary evidence: `tmp/restored-drop-obligation-qualified-final2-summary.local.json`,
SHA-256 0f1f85cfad9285e9c2797a1f199e0d268d920b7994dade70dff30ffc4bd67eee.
The primary verifies 541 raw native compile inputs, 2,222 LF-normalized frozen
public inputs and 1,229 production source/Makefile inputs against current source.
Native binary: 60dcf1226bdff76dc004239616db1c4e65792aeba916c1dc7e6df76fdf2407fd;
qualification metadata: e777e21a6c99a8f1503f2bc79a6c233a0a2a7ea4600877245a1ba406bfb8ce6a.
Strict production SQL: c356899040a940f9699423752d5fb082e75b2845cce386993e94f9caa039d385;
flatfile: dba9e47402788be76a2ca81ee09cd80bbd335fa6a59d5f4e94b67fd177892db0.
The summary retains engine, gameplay, owner and original-failure log pins.

Next: bind successful retained receipts to immutable full-literal room payload
and native ledger proof, wire replay registration and actor-independent exact
materialization, and release only after durable publication ACK in both ACK
paths. Actual producer/refusal/reconnect/copyover/two-cold-boot qualification,
other writer families, flatfile parity and the final current-head integrated run
remain open. Inventory stays 868 routes, 2,817 occurrences, 2,758 unique sites,
zero unmapped; coverage_complete=false and release=BLOCKED. All R1-R8 acceptance
gates remain required.


## Successful ordinary-drop receipts require retained payload and native history

Frozen published source `3e828dc0e` falsely accepts sixteen payload/native-ledger
fault cases on both SQL engines. Each observation uses actual direct duplicate,
independent cold-connection reconcile and production-pool duplicate paths;
nineteen exact repairs per engine restore the original successful receipt.
The three existing accounting-reference corruption controls correctly refuse.
Before-evidence artifact `tmp/room-retained-receipt-red-observations.local.json`
has SHA-256 2450b1fdc22dd5ece1f321f0e6751006b0a8b240c20bb73541a615044fcae3b2.

Successful ordinary-drop retained verification now binds the immutable original
operation's complete canonical literal payload and native ledger/reference rows
to the command and decoded result. Metadata is locked and byte/count bounded
before blobs are streamed; the blob pass uses the same current locking read,
including under a stale REPEATABLE READ snapshot. The verifier preserves its
transaction/session and reconnect prohibition, propagates SQL/allocation errors,
and does not consult later custody or the current season. It neither reconstructs
missing history nor changes legitimate rejected receipts. Missing/corrupt success
proof remains retryable through existing repository error mapping, retaining
original coordinator fences and durable journal work.

Both fresh guarded MariaDB and MySQL matrices pass all nineteen fault refusals
and exact repairs through the three real paths. Actual coordinator retry
exhaustion retains the original operation, every key fence and zero publication
ACK/checkpoints. Exact repair and restart of that same journal returns the
original result; only the subsequent successful explicit component ACK permits a
checkpoint. Persisted legitimate rejection remains unchanged without a room
payload. The public helper passes a two-connection stale-snapshot repair check,
and the original receipt remains valid after a real native UID transfer and
season advance. Original full-payload source, rollback, actual COMMIT-reply loss,
ACK and cold SQL connection assertions remain. Explicit fixture ACK is not a
player's physical publication or production replay-observer proof.

The immutable native candidate passes strict ASan/UBSan compilation under an
explicitly supervised 600-second component budget in 300.416 seconds. The default
runner budget remains 300 seconds. A separate single attempt with the original
300-second limit passes in 287.557 seconds using the same 596 pinned inputs and
flags, including the stale-read probe. Its executable is byte-identical to the
both-engine-tested candidate below. The supervised evidence remains separately
preserved rather than relabeled as the original-budget result. Fourteen native
pre-SQL target-guard negatives pass for each before/candidate binary. Candidate
binary: fc32b61b3fb65b270efed12ced60b98e3edac4ab303fe5dd0828c4a53e0510bb;
before binary: c8d91fbfa975a8b61da3b8d1bb2470386dfc69b75721a9b99d7eb65c0abd7d9b.
Supervised specialist declaration SHA-256:
9e496d4ce6bdded4243a2bb4b2417c16a97a361a39b827a7dfb0f0b82108cf9a.

Both corrected strict production backends pass with unchanged flags, -j2 and
600-second per-build limits. Current flatfile actual inactive creation, kit,
invitation/helper, hardcore, save, cold-restart and relog journeys pass in
147.367 seconds with unchanged waits. ASan/UBSan bounded payload admission passes.
The primary verifies 596 native raw inputs/current LF equality and all 1,232
tracked src inputs against production (including redis_key_registry.def and two
.gitignore inputs). Production SQL:
ba781300e5c1c371e6652bc5408e019519d74af7ff5c69975a929e5bd1be062e;
flatfile: cd36e10165fdcf96650f772c6e2e67a9465986d86b8c1dd2157b472e89660994.
Primary supervised evidence `tmp/room-retained-receipt-supervised-primary-summary.local.json`
has SHA-256 a1da3ddb4d8a21ac0d87506fe158fdcd38d332ee9bbf0a70b70e378f06a47403.
The initial RED driver lacked archived migration/script support and stopped
before services started; its log remains separate from supported dual-engine
runs. An initial pure-test invocation omitted the linker wrapper's required
TASK_LINKER_DIR, consuming its C++20 flag; the corrected unchanged invocation
passes. The original setup output is retained in task tool evidence.

One existing INSERT anchor shifts by one line after the new include; registry
and reproducible matrix retain 868 routes/2,817 occurrences/2,758 unique sites
and zero unmapped, without any route promotion. Coverage remains false and
release BLOCKED. All R1-R8 gates, actual producer/replay/physical publication,
copyover, complete current-head regression and measured workload remain open.
The authority plan also records the independently reviewed coin ACK-before-
physical-publication/null-replay/busy-retention defect as the next separate work.

Final original-budget primary evidence
`tmp/room-retained-receipt-final-primary-summary.local.json` has SHA-256
3e8c5c070c3145d21319e4e9fcf09ea053be2f105f9e6b25acdfbe92730afc66.
The final specialist declaration
`tmp/sql-room-retained-final-qualified-declaration.local.json` has SHA-256
b8ff1303c712deb9d6c9a8fc1f68c5fb055fa7ca4108ed0f3c8c8afebe2a235f.
These source-specific results qualify the candidate based on published
`3e828dc0e`; they do not qualify the newly fetched remote `5c3bc0957`.
That remote adds Chaos committed-message and applied quest-XP feedback changes,
including a common native header. Local history integration was rejected by
automatic approval review because the user's retained instructions prohibit
merging. The completed issue is committed locally; publication must preserve
both histories and awaits a decision about local Git integration. No divergent
push or alternate history rewrite has been attempted.


## Coin publication and ACK retention safety repair (qualified locally)

The frozen local 87de37985 owner reproduces five native component failures:
physical failure consumes the ACK before publication, later physical success is
never retried, missing restored publication capability ACKs without a native
pile, absent-actor replay loses the owner, and exhaustion loses the coordinator
obligation. Wallet-only replay remains a passing control. Successful simulated
ACK is irreversible and erases the controlled coordinator operation, matching
that production boundary; this is not a real database/journal/gameplay run.
The strict original 300-second before compile passes in 116.196 seconds with
binary a32b5def461250a0ae76f9720759589f3ba4dbf60703e22a84e3c7f7ea3dbbf4.

The intermediate repair also reproduced two partial-projection failures: a
shared-bank publisher throws after a wallet assignment, then a conflicting
receipt can replace the original and continue publication. Its original-limit
compile passes in 210.221 seconds; binary
7af7a35a8f77515a6a72101c44bb3cadb0ae172160a1a8d1fca9ea5e4757f979.
Original before, intermediate, setup-error and partial-failure artifacts remain
separate. Final native component, strict production, existing-owner and actual inactive
journey gates pass for this bounded repair.

The bounded repair separates verified schema-2 physical publication from
post-ACK notification. It retains the exact operation and receipt before the
first possible projection mutation, validates all wallet vectors first, latches
physical success through ACK retry, and retains blocked owners at exhaustion.
Changed receipt bytes cannot replace already-started publication. New schema-2
item-endpoint admission without an explicit verified publisher refuses before
coordinator admission; replay without that capability remains held. No existing
legacy physical helper is promoted as proof. Schema-1 composite callbacks and
the SQL wallet-root item-endpoint exclusion are preserved.

An initial copied-object strict SQL link failed because archived .d files
referenced a previous source tree's absolute header paths. All 32 affected source
objects per backend are explicitly rebuilt with unchanged production flags,
-j2 and original 600-second limits; the initial failed link is retained. The
root scheduling owner also initially lacked make in Windows; its unchanged
native Linux invocation passes all seven harness checks. The new native owner
is registered for serial resource-intensive regression execution.

This repair is not a typed native reconstruction or gameplay qualification.
Complete pile identity/topology/payload readback, staging both endpoints,
actorless replay, background-save holds, dependent corpse/saved-item projection
durability, copyover and active routes remain open. The existing physical helper
accepts newer metadata or unloaded owners without sufficient native proof;
never bind it directly as a verified publisher. A separate source-established
admission-failure cleanup gap also remains: the coordinator removes definitely
never-admitted operations before currency receives their terminal completion,
so publication ACK cannot succeed. That needs an explicit trusted disposition;
arbitrary ACK failure cannot authorize release. All R1-R8/full-current-head
qualification and release gates remain open.


Final frozen component source currency_transaction.c SHA-256
e4ee695bae9476d1360843a529d7f4ccdf374a7d5879f4abc23143e8f764bf2b
and header 4e8a0d709cbba08396555aabb1effa556eb21258dfccae9f36cab2615981bd7d
pass 14 native scenarios / 101 assertions in each SQL and flatfile mode with
strict warnings and ASan/UBSan. SQL first final compile timed out at the original
300-second limit while other builds overlapped; the timing cause is not proven.
One scheduled quiet attempt with identical source/flags/deadline passes in
189.160 seconds, binary
81e846e447d479bff8ac1d8ced2ed8421171137ff9cca90498e88796ca1789a8.
Flatfile passes in 89.078 seconds, binary
1a2153a1cb1c891edf3dd8c6f79259627dae58b29ea44fefbb18de9895bf8090.
The timeout remains separate; supplied-binary wrong-SHA checks, AST, formatter
and source equality checks pass. Declaration
`tmp/coin-publication-ack-retention-qualified-artifact.local.json` has SHA-256
a647b32f4d71766ff1ae757617b7de49317b3ea2a40c68d33e2c919a485db189.
The coordinator/physical publication controls in this component are supplied;
this is not actual journal, SQL execution or native pile reconstruction proof.

Final strict -j2 production SQL and flatfile builds pass their original
600-second limits after the documented 32-source header dependency rebuild.
SQL binary SHA-256
5b615f46d0adf6c9cb77c28581ccc576889585d56a48c1108f30e87f78844ae1;
flatfile 64cd9c0784d063f92e2aaf50435302cd2f38376508ff706f01be0c5042ca9f22.
All 1,232 tracked source inputs match their native snapshot after normalizing LF;
raw native bytes are pinned separately, including the CRLF .gitignore inputs.
Source readiness artifact SHA-256
60826fb9c2cbb2128a099cf13b82106956e1cc05fa9b659d4bc43fef2aee83e4.
The initial raw-vs-LF checker disagreement on src/.gitignore is preserved as a
checker setup error, not a production source mismatch.

Fresh remote inspection now finds experimental-accounting at
100bee62f2b6111e5037984a970c2cddb62158ec, with additional death-recovery visibility
and shop quantity changes. Those incoming changes, including common completion
headers and coordinator code, are not integrated or qualified by these results.
Local integration still awaits the requested decision after automatic approval
review rejected merging under the user's no-merge instruction. No divergent
push or history-rewrite workaround has been attempted.


Final existing currency retention owner passes all 106 ASan/UBSan scenarios,
and input-queue owner passes both SQL and flatfile variants, against the exact
final production snapshot. Actual inactive creation/save/coldrestart/relog passes
in 134.989 seconds with unchanged prompt waits. The actual area coin-pickup owner
builds its inspector within the original 180-second deadline and passes
`get coins statue`, `get all.coins statue` and `take all statue`: each credits
exactly ten platinum, removes active room custody and refuses duplicate value.
Both production binary hashes remain unchanged. These are actual inactive
behavior checks; they do not establish active/native pile publication or replay.

Primary final summary
`tmp/coin-publication-retention-final-primary-summary.local.json` has SHA-256
1ebdd173188249c11637b195591c23ecc03639e290c164b06008b4b871a37ed4.
The reproducible matrix check, all 14 accounting fixtures and 54 writer coverage
contract checks pass: 868 routes / 2,817 occurrences / 2,758 unique sites /
zero unmapped; coverage remains false and release BLOCKED. Full-current-head
broad regression, active native publication/reconstruction, replay/save/copyover,
measured workload and every R1-R8 completion gate remain open. This solved safety
issue is a separate local commit; normal direct publication awaits the explicit
local Git integration decision and combined-source qualification of incoming
100bee62f. The next issue has an immutable before-state freeze for actual
coordinator/journal admission failure and domain cleanup qualification.


## Definite admission failure disposition (implementation and qualification in progress)

Actual currency/coordinator/journal before-state qualification reproduces six
native RED cases: coin and bank ENOSPC, partial append with successful rollback,
and append fsync failure with successful rollback. After native delivery the
real coordinator has removed the original fence and journal record, with zero
worker execution and projection, while the domain owner remains pending and
rejection callbacks remain zero. Four uncertain-admission and durable rejection
ACK/checkpoint-repair controls pass. The strict ASan/UBSan before binary
4fcfe55081834c357b6879588e74c115d8b321351efe42be8be786c47a38074b
compiles in 110.370 seconds within the original 300-second budget. Before runtime
`tmp/admission-currency-before-red-runtime.local.log` has SHA-256
ab27ed8ffdc55f5f418f0e08ee4356c3225a21a1620dac5aea596ff11f83994c.
Only the worker/domain-world effects are controlled; the actual coordinator,
journal admission, pulse, retained ACK and private filesystem are used. No real
SQL engine or player journey is claimed.

The initial four-file implementation appends an in-process disposition with a
safe execution default; only proven definite admission refusal marks never
admitted. A bounded no-allocation validator requires terminal rejection, a real
error, zero durable revision/start/stage/result size and all-zero result bytes.
Commands, journal frames and stored receipts are unchanged. Valid never-admitted
receipts bypass only the impossible publication ACK, preserving rejection/domain
cleanup. Unknown or changed disposition stays held, without projection; entire
incoming batches are validated before item retry or collector invalidation, and
reconnect cannot clear the separate disposition latch. Same-batch contradictions
cannot be repaired by a later duplicate. Craft staged output disposal remains
required; never-admitted craft cannot publish or acknowledge progression.

Read-only review finds no blocking defect in this initial patch; changed-line
clang-format18 fixpoints and the seven root harness scheduling checks pass.
The new native regression runs serially. Complete item/craft RED and both-mode
final owner/recovery, existing owners, strict production and gameplay checks
remain pending. This is uncommitted implementation, not a solved issue or any
R1-R8/release promotion. The separate craft ACK retry completion/progression
cleanup bug remains open.

Item/craft actual before-state owner adds eight definite-failure RED cases
(ENOSPC, partial append, fsync rollback and real quota for each), with four
uncertainty exact-ID recovery and retained rejection ACKrepair controls passing.
Before binary c03ef2c6b0f563235ba4f2c2c08202bd4b340e8b2d944d3c97bd0aa943def759
compiles in 79.383/300 seconds. Runtime log
`tmp/admission-item-before-red-closure-runtime.local.log` has SHA-256
d81c8273d9cb91c10ccc96cc4f31b2ff8a129c99eeab5be4b8d13e5d327fe92f.
The before craft failure disposes its staged output once but emits EBADMSG and
remains pending. Its separate normal ACKretry control visibly calls completion
twice; that control qualifies retention/checkpoint repair, not notification
exactly-once. The latter remains the next independent issue.

Final candidate checks use an immutable completed freeze. An initial native
attempt raced that freeze copy and read the old header; it is preserved as a
setup error with no artifact. Corrected source equality must finish before
compilation. No green qualification is claimed yet. The authority plan also
records the source-established legacy partial snapshot competing projection
and startup/save-quarantine dependency before ordinary-drop recovery wiring.
All four frozen final domain/coordinator owners now pass: currency SQL-header
36/36, currency flatfile36/36, item/craft SQL-header34/34 and flatfile34/34,
140 private-journal native scenarios total. Original strict ASan/UBSan compile
budgets remain300 seconds. SQL currency binary
54aa15149a7df455e11f8f6565eeaf159a99bd5a352b7b716d8619e75f38d329
(72.077s); flat currency d32edccfb1cea93ff553d4865d75211b0c32e2ec8e8ebe0d5ad4b2867d84ec1e
(63.567s); SQL item4b0148e211453775a6a22916d1135d40f56ac50b309d223e41603d85512f6503
(81.680s); flat item34c31d02074d2512fc64fc949563679a5d9208043a1cc9c9475c2d5ce4d3b5af
(92.240s). The actual coordinator/journal/pulse/fences/checkpoint ACK are linked;
worker execution effects and world leaves are controlled. No SQL execution,
active physical publication, gameplay or complete release qualification follows
from this component result. Production freeze, existing native owners and actual
inactive gameplay qualification remain pending. The54 writer contracts,
2727 site checks, matrix reproducibility and14 accounting fixtures also pass;
coverage remains false and release BLOCKED.

The final native declaration is
`tmp/critical-admission-owner-release-qualified-artifact.local.json`, SHA-256
9d525646dca0a694855b4ef662df666ad02999ec00f8d6dd446cda2b62a72a09.
Every native/header/fixture input matches current source. The initial broader
freeze includes a derived Python cache subsequently refreshed by the supplied
wrong-SHA guard; that cache discrepancy and initial manifest remain preserved.
The separately named source-only manifest excludes bytecode. No full-tree
immutability claim follows; source/test/binary inputs are unchanged.

Production preparation initially refused the legacy shared pfile dependency
name `core/files.pf.d`, which maps to `core/files.c`, before any production
compilation. Corrected source mapping and completed equality verification identify
117 SQL and118 flatfile sources affected by the common completion header. All
1232 source inputs and five modified test inputs are frozen before existing
owner tests or production builds. This setup refusal is separate from native
qualification; budgets and production flags are unchanged.

The ten existing maintained checks each pass, including both currency queue
modes,106 currency retention scenarios, actual journal admission/capacity/fault/
uncertainty owners, item publication, progression and native restore decoding.
The coordinator identity/order check is a source contract, not a native runtime.
The driver itself did not pass: primary changed that executing shell file while
adding bytecode suppression, causing a repeated successful craft restore and
subsequent parse failure before either final coin check. Driver failure remains
in `tmp/admission-disposition-owner-driver-failure.local.log`; no production
source defect is inferred. Remaining coin checks use a separate stable driver.
Their initial launch found the older inherited QA tree lacked the new coin test,
refusing before compilation. That original log is preserved separately; the
maintained test was copied, its1128bd82e9e05ef451bc749ea35bd18a33ad79e1436c8b0e7f0d2e28ba351aef
SHA-256 verified, and corrected checks use separately named `ready` logs with
the unchanged compile/runtime deadlines. Executing drivers must remain unchanged.

Both final strict production builds now pass -j2 with the original600-second
budget after the complete shared-header rebuild: SQL162 seconds, binary
SHA-256 d20b20c129b93a032c322294e885196be88780f0e85c167c0502567691dbe505;
flatfile166 seconds, binary0db5f32ef9d2509dbc963cbd55a69e7fc05e839ed6f771e8e4434a37422a3de9.
All1232 tracked source inputs agree after LF normalization, with raw native bytes
pinned separately. These are production build results, not actual SQL execution
or combined incoming-remote qualification. Actual inactive creation/save/restart/
relog and three area coin-pickup commands are the remaining bounded gates before
committing this issue. The separate craft notification/ACK cleanup regression
is preparation only; read-only review requires real failed ACK counts, actual
progression-map erasure, initial physical publication, retained reentrant owners,
native actor retirement controls and complete source/artifact pins before RED.


### Definite admission cleanup: bounded issue qualified locally

Actual inactive creation/save/coldrestart/relog passes in 130.116 seconds with
unchanged prompt waits. All three real area pickup commands pass after the
inspector builds within its original 180-second budget: `get coins statue`,
`get all.coins statue`, and `take all statue` each credit exactly ten platinum,
retire room custody and refuse duplicate value. Both production binary hashes
remain unchanged. These are inactive journeys, not active pile publication.

Primary summary `tmp/admission-disposition-final-primary-summary.local.json`
has SHA-256 7244cacb8e799376c761e9938729e9215b4b6f5d370ddb939fea6e4c0b57c575.
The summary checks the 140-case declaration, all 1,232 current source inputs,
two exact strict binaries and every existing-owner/gameplay evidence marker.
Source contracts and native runtime evidence are labeled separately. Normal
accounting contracts, 54 writer coverage tests and 2,727 writer site checks pass;
868 routes / 2,817 occurrences / 2,758 unique sites / zero unmapped still means
coverage=false and release BLOCKED. Failed before cases, copy/cache/dependency
setup errors, interrupted driver and missing inherited fixture remain separate.

This issue is ready for its separate local commit. Normal direct publication
awaits the pending local Git merge decision after automatic approval review
rejected that operation under the user's no-merge instruction. Incoming
100bee62f is neither integrated nor qualified; no history workaround or divergent
push occurred. Next is separate craft notification/progression cleanup after
normal ACK retry. Ordinary-drop producer/native publication/replay/save/copyover,
independent complete audit, route/backend/workload and all full R1-R8 gates stay
open. Accounting remains inactive, SQL wallet-root item-endpoint exclusion and
all safety gates remain, and the declined inactive spell-path change is untouched.


### Live publication contract boundary correction

The 19-case live movement source-contract suite exposed an obsolete text slice:
it searched for an ACK guard without the explicit never-admitted condition,
which 3ad1f91bb intentionally added. Its failure was a missing substring rather
than a runtime failure. The contract now extracts the actual nested failed-
publication block with the existing brace-aware helper and still requires
retention with no owner erasure. All 19 cases pass. This is a separate fixture
repair; no production code, route qualification or accounting activation follows.
The original failure log and corrected source-contract log remain protected in
`tmp/craft-ack-test_live_item_movement_contract.py.local.log` and
`tmp/craft-ack-test_live_item_movement_contract-corrected.local.log`.

The craft ACK-retry implementation is separately in progress: actual native
before evidence now executes all 19 valid scenarios and reproduces duplicate
completion, skipped real progression-map cleanup and unsafe notification owner
boundaries. Attempt1/2 recipe fixture admission errors are preserved separately,
not counted as production failures. Source review additionally requires immutable
receipt binding before physical/progression effects. Final native/production/
gameplay qualification of that fix is not yet claimed. Local publication remains
blocked by the pending local Git integration decision under the no-merge rule.


### Craft completion: final qualification in progress

The actual native before-source owner now reaches every one of the 19 valid
scenario paths. Original strict sanitizer compile passes in 83.443 seconds of
300; binary b204a12a75302c84f68916ee4a5e7abf9752b456d6ec9a4f1342bdbd8fb1882f.
All 19 cases fail semantic assertions, with 621 passing control assertions and
no sanitizer fault. Applied recipe effects occur once but completion runs twice
and the real progression attempt survives ACK repair. Declaration
`tmp/craft-publication-ack-before-attempt3-artifact.local.json` has SHA-256
6974fe63ef35e6f94862db6543ac540c9ea75ef0d208e20156e59a9302471213.
Attempt1/2 recipe fixture admission failures remain separately preserved; they
are not valid recipe production failures. The final 30-case fixture removes the
artificial rejected progression seed and preserves real applied map-erasure
proof. It adds changed/malformed receipts during ACK retention and progression-
save waiting, contradiction batches, exact repair, and equivalent durable
success with changed diagnostic timing/attempt fields.

The draft finalizer waits for the original ACK (or validated never-admission),
then extracts the domain owner before independent progression cleanup, recipe
notification and business completion. Native actor registration is required;
post-hook actor lookup uses the original runtime identity. Independent readiness
and authoritative receipt binding preserve effects across retries and refuse
changed outcomes before publication. Arbitrary pre-ACK progression hook throw/
reentry is outside this bounded proof; the current production progression hook
catches allocation failure and has no identified item-owner recursion.

Both strict production builds pass -j2/original600 after rebuilding the changed
native item unit: SQL13 seconds, binary
92f0e7ea6419f03d69b884a014e69efe90e09d6cb2c0593805682de88267b19f;
flatfile11 seconds, binary
8b23635f306846fbece9f32c300119e9d7a76ef2f83b380fb6b3beccf3ae7a95.
All1232 source inputs are verified before testing. Actual inactive creation/save/
cold restart/relog passes in136.878 seconds; all three unchanged area coin-pickup
controls pass after the original180-second inspector budget. These controls do
not qualify active craft or native drop publication. Five maintained final test
inputs and the expanded30-case/21-unit harness are frozen before native compile.
Runner SHA-256 a44539745b2ce6275e2ac693ae7dadfddb710381dc7f5e57f146f00e785120fa;
generated harness c1f3f83295c599aa335c77f1576d9018fb7fdd734c6436d2ab5543163685e3ed.
Final native SQL-header/flatfile and existing owner qualification remain pending.

The seven root scheduler checks pass on Linux; a Windows launch lacked make and
is an environment failure. The19 live movement,54 writer coverage,2727 site,
craft module/persistence contracts and Linux route-evidence checks pass. Windows
route evidence hit its python3 App Execution Alias; its Linux rerun passes.
Matrix reproduction passes with unchanged868routes/2817occurrences/2758unique,
zero unmapped, coverage=false/releaseBLOCKED. A fresh explicit remote read still
finds100bee62f; local history has four commits and21 incoming commits since the
common base. Local Git integration authorization remains pending; no merge,
history workaround, divergent push, activation or production mutation occurred.


Expanded before-source proof preserves30 valid native semantic failures,
861 passing control assertions and242 failed assertions, with no sanitizer fault.
Original strict compile passes82.574/300 seconds, binary
d3683ec6f45b78995a78203d3084d20e362634546174d67d12e50f5a9e988ecc.
Declaration `tmp/craft-publication-ack-expanded-before-artifact.local.json` has
SHA-256 f19cdded9cede34cc3e38c74416c168aa170bc96a0766da8f25d99083562fbd6.
The unseeded rejected recipe does no progression work; applied recipes prove
actual map erasure. No recipe admission setup error is counted in this cohort.

Final SQL-header component now passes all30 native cases and995 assertions with
no sanitizer fault. Original strict compile83.966/300 seconds; binary
0afd9351ecfb1ee7969c68409a3c3770d09017ca61dcc6afb1ac033c250bb6c4.
Actual journal ACK returns false four times then succeeds once; recipe physical
and progression effects, cleanup and business/recipe notifications each occur
once. Changed/malformed authority, full contradiction batches, progression-save
waiting and exact repair controls all pass. Final flatfile and existing native
owners plus actual mortal Craft/Forge/copyover/restart proof remain pending.
These native fixtures inject the matching saved hook; they do not prove a real
SQL save completion or active gameplay qualification.


Final flatfile component also passes30/30 cases and995 assertions: original
strict sanitizer compile77.102/300 seconds, binary
024357a08b3a2a82542ae46d0e21013fbc06aa57bc461cb6346cec6a2e3db2b1.
The immutable combined declaration
`tmp/craft-publication-ack-qualified-artifact.local.json`, SHA-256
1d0d990ca81a596f367fab401e96a7995fcf088ee4d85ccb129ef1e37dd82b5b,
records60 native passes/1990 assertions, exact pre/post native/harness/runner
pins and16 supplied-artifact guards (14 refusals/two valid controls). No service,
SQL, actual save completion, gameplay restart or full route proof is implied.
Both current item/craft definite-admission owners also pass34/34 per build mode,
including disposition-conflict repair without repeated callbacks or output
disposal. Remaining existing owners and real recipe journey are still running/
pending before this separate issue can be committed as solved.


### Craft ACK-retry issue: final local qualification

The bounded craft completion/progression-cleanup issue is solved locally. The
final immutable component declaration above remains unchanged:60 native passes,
1990 assertions and16 artifact guards. All68 existing item/craft admission cases
pass. Existing publication-retention, progression, progression restore, recipe
transaction, input queue, ACK checkpoint, copyover-drain source contract and real
flatfile craft/pouch persistence owners all pass unchanged assertions and budgets.
The copyover-drain source contract alone is not an actual copyover journey.

The actual inactive mortal Craft/Forge journey now also passes on the final strict
flatfile binary: retained pouch, material/tool UID conservation, exact XP,
copyover and two cold restarts. The inspector builds in90.445 seconds within its
original600-second budget; recipe owner prompts/recovery waits are unchanged.
The two strict production builds and inactive creation/relog/three-pickup controls
above remain verified. The complete1232 raw native source inputs remain unchanged
through final qualification, and maintained final test pins are checked again.

Primary aggregate declaration `tmp/craft-ack-final-primary-summary.local.json`
SHA-256 27327027c8cb604f46067ed0a2364b73ab0f3f673f0aa08e10c0793f3acae548 verifies the component declaration/log hashes, current
source/test pins, native binary/metadata hashes, strict production binary hashes
and all native/gameplay owner pass markers. It preserves invalid recipe setup
attempts separately from valid19/30-case semantic failures. Private artifacts
and logs remain protected locally; the source and maintained fixture are committed.

Native saved() callback injection does not prove actual SQL save completion.
Arbitrary pre-ACK progression hook throw/reentry, active accounting, ordinary-drop
producer/replay/save/copyover, combined remote source and current-head broad
qualification remain open. Coverage remains false/release BLOCKED; no route or
R1-R8 acceptance gate is promoted. The separate eleven-case partial-save fixture
is preparation only until actual-engine RED/GREEN and recovery checks pass.
This solved issue is a separate local milestone; normal publication still awaits
explicit local Git integration authorization after automatic approval review
rejected merging under the retained no-merge instruction. No history workaround,
divergent push, accounting activation or production-data mutation occurred.

### SQL reconciliation fixture result description

The private strict native preparation for the new eleven-case partial-save
fixture first failed before any SQL scenario: the maintained result-description
switch omitted player_save_journal_result::quarantined_pid. The helper now names
that existing result; no production source or assertion is weakened. The original
fixture/snapshot/scripts and compile log remain preserved as a setup failure,
not a semantic RED. A new immutable candidate passes the exact16-unit strict
-Wall/-Wextra/-Werror ASan/UBSan compile in58.256 seconds. Native binary SHA-256
31bf1ff01c080a3b7c9801a77caaec2842b5939ea702fd773ffe71c2e785f67f.
The wrapper adds new supervised300-second compile/120-second runtime limits;
the maintained main originally has neither. Actual SQL cases remain unmeasured.
This one-line fixture repair is a separate solved milestone; the unfinished
partial-save regression additions remain outside its commit.

A fresh remote read is33d23aeec438b7fc99911a50a75614468b1f57df, whose additional
change consolidates issue tracking documentation. Its #490 acceptance retains
all R1-R8; superseded issues are not completion claims. That remote source is
still unintegrated with the local qualified milestones; local merge authorization
remains pending under automatic approval review's retained no-merge restriction.
The primary performed the required notebook-only fallback after the curator
agent could not start at the thread limit. Both scoped owning notes stay below
20KB; existing unrelated oversized code-research and derived index drift remain
reported by the read-only overlay validators.

### Partial SQL item-save replacement: measured failure, fix in progress

Both fresh disposable MariaDB10.11.14 and MySQL8.0.46 targets reproduce eight
semantic failures using the same immutable16-unit native fixture and exact31bf
binary above. Valid inventory-only replacement loses equipped-container children;
stale room/foreign/destroyed inventory payloads reinsert; omitted selected children
and cross-PID cascade closure are accepted instead of guarded. Valid equipment
replacement and both scoped after-DELETE/after-root-INSERT rollback faults pass.
Existing full-save and legacy controls pass. Each refused-frame test requires
preceding victim STATUS rollback, exact quarantine evidence and unrelated PID
STATUS/revision progress through same-journal replay and cold connection/restarts.
There are16 measured semantic failures and6 passing partial controls total.

The independently verified declaration
`tmp/partial-save-before-both-engines-qualified.local.json` has SHA-256
14c16d281bb297563c1005927b24e3901623ee1c28be1729d3d4270287b813c9.
It checks immutable1232 native source pins, exact fixture/harness/16-unit closure,
binary/metadata/log hashes and whole owned-schema/server teardown. The v1 strict
fixture enum failure and v2 MySQL trigger setup exit remain separately preserved;
v2 MySQL's eight valid failures are not counted as a complete11-case run. V3
adds SUPER only to the synthetic user on its fresh owned MySQL server because
binary logging requires it for fault triggers. Binary logging is unchanged; no
production-account permission qualification is implied. All55 migration SQL
scripts run through maintained bootstrap; this does not qualify migration receipt
history. The private300/120 limits remain newly supervised, not maintained gates.

The production fix is in progress: bounded complete custody/native forest and
cascade verification, approved legacy root-position compatibility, selected
physical boundary deletion and partial-only restitution scope. Additional valid
legacy/inline-coin/restored-leaf/restitution and malformed/bounds controls remain
required. The synthetic room after-state used here is not a real pooled schema2
drop. An independent opt-in native room fixture is being extended for real
coordinator/journal/pool drop compatibility, READ COMMITTED save/drop orders,
actual lock-timeout witnesses, foreign-child FK order and independent PID progress.
No GREEN, gameplay route, current-remote integration or full R1-R8 gate is claimed.

The independent partial-path review found two newly introduced allocation escapes:
the selected native deletion-ID SQL and restitution root-filter assembly were
outside allocation guards. An escaping allocation could retain the ordinary
borrowed SQL connection and transaction. The fix now being prepared must return
a failed query result through the existing rollback path, preserve result
ownership, and exercise actual native allocation failures in these scoped SQL
preparation windows. The earlier f810/0df candidate remains unqualified and
preserved; no passing source review alone closes this issue.

The immutable expanded32 BEFORE fixture compiles with strict warnings and
ASan/UBSan in58.020 seconds; binary SHA-256
f5aa0dbb29ba2012a187d3a855fd76f4b180855a462c90ac2a2ee9eb5fdaa840.
Both engines reproduce the original8 RED/3 passing controls, then exit2 while
seeding the first additional case. None of the21 added cases executed. The
MariaDB043b3ad9 and MySQL3002b7ec logs and receipts retain this setup failure;
whole owned-schema/server teardown succeeds on both targets. The fixture seed
failure must be diagnosed before those additional contracts can be qualified.

Independent source review of the opt-in six-case pooled drop/direct save race
fixture e24801d4 finds no remaining concrete blocker. It uses READ COMMITTED,
actual pool/coordinator drops, independent direct repository saves, precise
lock/FK witnesses, and raw-byte protected-state comparisons. Its explicit ACK
helper is not native gameplay publication. The first private source preparation
omitted the maintained codec SQL escape stub and failed before compilation;
that incomplete snapshot remains preserved. A new snapshot includes the exact
68-unit maintained source collector closure. Native compilation and both-engine
execution remain pending; no race or route acceptance is promoted.

The same e248 room fixture now has actual paired pooled qualification. The
before-source68-unit strict ASan/UBSan build passes its original300-second gate
in172.830 seconds (binary768bb4bf). Both MySQL8.0.46 and MariaDB10.11.14 measure
the same five RED cases and one passing parent-lock/FK control. The reviewed
8b989 repository candidate compiles in201.214 seconds (binary59242c69), then
all six cases pass on each engine under the original120-second runtime gate.
This qualifies those READ COMMITTED pool/direct-save orders and retained drop
receipt compatibility, not native gameplay publication or production activation.
The first driver's overlong advisory-lock schema names, second driver's port
precheck exit, and incomplete source collector are separately preserved setup
failures. No assertion or native runtime limit was weakened.

The34-case allocation fixture against the preserved f810 pre-allocation-guard
candidate passes all11 original and21 additional item-tree contracts on MariaDB,
then both allocation cases fail across51 injected ordinals. Fifteen native
bad_alloc exceptions escape with transactions held; two other faults return
non-ENOMEM results without escaping. This baseline is distinct from the original
ec366 pre-forest implementation. MySQL passes those32 contracts but the first
allocation oracle used an unavailable system variable and exits2; that run is
not full allocation qualification. The new af2b7ee fixture uses successful
native SELECT1 result/status observations, proves actual begin/rollback0-1-0
transitions and requires an active transaction at every allocation window.
Independent source review passes; exact before/after native builds pass strict
warnings/sanitizers. Both-engine allocation execution remains pending. A later
driver preparation exceeded the Unix socket path bound; both failed server
starts and owned teardown are retained, and only the private directory prefix
is shortened for the next runs. Source8b989 remains unchanged throughout.

### Partial SQL item-save component: final native qualification

The reviewed source8b989b3c1c2a3ee0e34f7f4ff82b7830607703e23702ce9c4037fe39dff911cd
and portable fixtureaf2b7ee5288540805bf0852bd8c4161316fa7ed3fe4bfe8c86ed6f8dd0e717b1
now pass all34 direct repository cases on each engine, including both bounded
allocation sweeps. Each engine checks51 actual injected ordinals: observed active
transaction before the fault, no escaping exception, terminal ENOMEM, unchanged
durable revision/status/protected rows and actual transaction closure afterward.
The same fixture against f810 passes32 item-tree contracts then both allocation
cases fail on both engines. Original pre-forest ec366 failure evidence remains
separate. Six pooled-drop/direct-save cases per engine also pass on the final
source; drops use the real pool/coordinator, saves use independent direct SQL
connections, and fixture ACK is separate from gameplay publication.

The immutable primary declaration
`tmp/partial-save-qualified-component.local.json` SHA-256
71055b7b37da7546043217d559a6992c3c110f0900f41127a8de4c837b1a2cc1
rechecks exact source/fixture/harness/metadata/binary pins, paired engine results,
portable transaction witnesses, owned teardown, and all1232 native source entries.
It records68 passing direct cases,102 allocation ordinals and12 pool cases.
Both strict production builds pass their original600-second/-j2 gates with the
changed repository unit actually recompiled: SQL20 seconds, binary SHA-256
2b1e5e09a7ddf6b0340950d33b5dceed91cd15ccbc856baf76c2cd1037274482;
flatfile19 seconds, binary SHA-256
15a5daa3274ca69737f566a4ec1ce1a0e060731e538c6887a6d3566000100374.
The verifier's original make-relative source-path mismatch remains a failed
verification attempt; its corrected exact compiler-command matcher passes.

The maintained actual inactive SQL gameplay journey passes on fresh MySQL8.0.46
and MariaDB10.11.14: elapsed/quiet/repeated saves, link loss, quit/restart,
death/reload, crash recovery and preserved-connection copyover, followed by the
complete34-case native reconciliation and spell-receipt checks. V1 fails before
boot because the journey's restricted environment drops TASK_LINKER_DIR and the
old shim's empty -L consumes -std=c++20. V2 uses a new private compiler adapter
with a fixed linker path; source/assertions/flags remain intact. Both original
failed attempts are preserved. No declined inactive spell-path change occurred.

Nine targeted maintained source/native owners pass: custody write guard,
synchronous item-state contract, pet loading, restitution accounting, dynamic
item hydration, death selector, save worker, save pipeline, and journal lifecycle.
The captured-journal quarantine owner exits at its required four private input
arguments; it is unrun, not passing or a semantic production failure. Current
accounting30, writer54, evidence-linking2 and root-harness7 checks pass; the
Windows evidence check's missing python3 App Execution Alias failure is preserved
separately from its passing Linux run. The one moved census identity is reanchored
after exact identity/multiplicity checks:868 routes/2817 occurrences/2758 unique
sites/0 unmapped, evidence unchanged, coverage false/release BLOCKED. Actual
inactive flatfile creation/save/cold restart/relog qualification passes in130.849
seconds with the current1232 source and strict binary pins preserved.
The immutable final milestone declaration
`tmp/partial-save-final-milestone.local.json` SHA-256
ca67238afe36893277dabb1580b4688fe9bd2ba5b652e7e5ed3de6889a8ead14
links the component proof, both completed SQL gameplay logs/owned teardown, the
flatfile journey and nine maintained owners, with the captured-input limitation
explicit. Final clang-format18/source-fixture pins and generated matrix check
pass; the broader release remains BLOCKED.

This closes a partial-save prerequisite only. Whole-save exception safety,
pool-lease allocation sweeps, captured staging/backup qualification, native
ordinary-drop producer/replay/restored-save/copyover, active accounting, combined
remote source and current-head broad qualification remain open. No R1-R8 route
or release gate is promoted; publication still awaits the pending explicit local
Git integration decision under the retained no-merge restriction.

### Quarantined dispatcher regression fixture repair

The maintained extracted dispatcher fixture failed compilation because its
literal inventory helper references STRUNG_KEYS/DESC1/DESC2/DESC3 without the
owning core/defines.h header. Adding that one include repairs the fixture; no
production function, flag value, assertion or compiler/runtime budget changes.
Its original sanitizer owner now passes all ordinary, directory-sync retry and
archive-plus-deque-allocation-failure cases: exact quarantined bytes survive,
healthy backlog proceeds, no fake ACK, and shutdown returns within8 seconds.
The failed attempt remains in the private evidence. Declaration
`tmp/quarantined-dispatcher-header-fix.local.json` SHA-256
01f3dc948577f78f29f4f434a28e1b71a3c89e09c0e9198957a369bbadc0379f
pins the before/after fixture and failure/pass logs. This is a repaired regression
owner, not current broad qualification or an accounting route completion.

### Resident persistence diagnostic fixture repair

The actual resident diagnostic now reads literal checkpoint metadata, but its
extracted staff-command fixture omitted that type/array/lookup and failed to
compile at find_literal_inventory_locked. The fixture now extracts the real
checkpoint declarations and lookup alongside the real diagnostic, rather than
substituting an admission answer. New held/unheld/other-PID controls pass with
the existing terminal fence, try-lock, privilege/parser, bounded witness and JSON
checks. Original compiler flags and15-second runtimes remain unchanged; metadata
leaves remain controlled and no production function changes. Declaration
`tmp/persistence-diagnostic-fixture-fix.local.json` SHA-256
b114cecbb3baf0568fcb8de9ec0e1841f2a84113973d03d617aa5ec5418e5b65
pins source and failed/passing evidence. This repairs an applicable regression
owner only; it does not qualify actual SQL, gameplay or the current broad gate.

### Worker parking without save failure or identity replacement

Restored authority cannot use ordinary retryable failure to hold a save: retries
eventually quarantine its PID, while undispatched replacement can discard the
original request during a wake-to-dispatch window. The worker now recognizes the
appended deferred outcome6, retains active/pending bytes and revisions without
completion, ACK, retry increment or quarantine, and permits one explicit resume.
A sticky original flag prevents replacement even after wake or an ordinary retry;
only pending work coalesces. Resume allocation refusal retains the parked owner,
and a bounded resident deferred-PID count appears in staff diagnostics. Normal
pipeline/repository callbacks do not use the new outcome, preserving inactive
behavior. A wake before parking returns false; its caller must retain/retry it.

The identical new sanitizer owner links the real worker, revision, codec, journal
and observability units. The before source has seven semantic RED cases and one
real journal-ACK refusal/repair control PASS. Afterward all eight cases pass in
SQL-header and flatfile modes, including concurrent wakes, protected original
identity, pending coalescing, set/deque allocation faults, more-than-retry-limit
healthy-PID progress, exact journal reopen and explicit warm-reinit resume. SQL
mode initializes the real client thread but does not execute a SQL repository;
the application callback is controlled. Supplied-artifact wrong-SHA/backend/source
guards pass. New owner budgets are300-second compile/120-second aggregate runtime.
Its frozen component declaration is `tmp/worker-deferral-qualified-artifact.local.json`,
SHA-256901d8f8935964e04bd86eb2cc3bc7e7d8d4da417d212f28e15ce597c99f61ddb.

Both strict production builds pass with unchanged600-second budgets and-j2. All
49 server consumers of the changed worker header recompile per backend; the SQL
copy's additional pfile-only consumer is explicitly compiled through its original
Makefile target. The initial assertion wrongly included that separate tool in
the server target and remains preserved. SQL binary SHA-256
b296c920a3a6842b97a327f793f8794d67572a5d19d30bd12975930301e4259b;
flatfile80eccf1f66cc90722bbf12b13502a3717bec7187c63e1b4019e50b5075982eee.
Actual inactive gameplay passes elapsed/quiet/repeated saves, link loss, quit/
restart, death/reload, crash recovery and live copyover on disposable MySQL and
MariaDB. Both owned database instances/schema teardowns and port-rebind checks
pass. Flatfile creation/save/cold-restart/relog passes in130.518 seconds. Its
initial staff-fixture link failure remains preserved; the corrected wrapper uses
the existing private compiler/linker libraries without changing maintained
assertions or budgets. These journeys exercise normal callbacks, not restored
save parking with an actual SQL apply.

Existing worker, pipeline, journal, repaired quarantine and diagnostic owners,
phase01 recovery/load,14 accounting validations,30 accounting tests,54 writer
contracts,2 route-evidence and7 root scheduling tests pass. Clang-format18
fixpoints, generated matrix and whitespace checks pass. Final declaration
`tmp/worker-deferral-final-milestone.local.json`, SHA-256
c1b7d3fc7c63a361da2ed1b2c47763f69fee0ba4749247c9313631fe3205acd4,
verifies1232 current native inputs and frozen source/test/binary/log identities.

This closes the worker-only prerequisite. Startup suspension, registration/apply
ownership through checkpoint, selective exact-frame journal replay, affected-PID
hydration, stale-frame disposition and original publication ACK remain open.
Separate source-established admission/retry/promotion and completion-queue
allocation gaps are not repaired by resume's guarantee. Ordinary-drop native
producer/replay/copyover, combined remote source, current broad qualification and
full R1-R8 remain open. Inventory remains868 routes/2817 occurrences/2758 unique
sites/zero unmapped, coverage_complete=false and release=BLOCKED. The refreshed
remote is6841487786f1f17be3685c53a42ea4860c0909ae; publication still awaits local
integration authorization under the retained no-merge restriction.

### Selective journal deferral with exact-frame preservation

Journal replay treated the worker's new deferred outcome as a terminal failure.
It now appends replay_deferred result10 and retains every original frame for a
held PID, skipping its later callbacks and removing both earlier ordinary
revision proofs and exact death/quest/spell/craft proofs from that pass. Sorted
PID traversal uses an allocation-free scalar marker; unrelated PIDs continue
and checkpoint. A real checkpoint error takes precedence over deferral. The
preapply collection also catches allocation failure before invoking any callback
or checkpoint. Normal production callbacks still do not return deferred, and
the existing result==ok global replay/load gate refuses result10.

The identical final19-case sanitizer owner links actual journal/codec/observability
with controlled repository application. Before source has16 semantic RED cases
and three unchanged failure controls PASS. After source passes all19 cases in
SQL-header and flatfile modes. Both contain60 injected scan/collection allocation
failures, reopen/retry, then uninjected success. Before ordinals0-49 safely refuse
and recover; ordinal50 escapes collection. Other cases cover prior durable99 and
exact operation/death proofs, duplicate frame IDs, multiple held players, real
concurrent append, mixed retry/ambiguity/terminal/runtime failures, real pre-rename
fdatasync failure and post-rename directory-fsync repair with an unrelated exact
quest ACK. Raw retained headers, timestamps, IDs, payload and CRC stay exact.

Attempt1 omitted the RENT_DEATH-owning core/files.h include and failed fixture
compilation; it is not semantic RED. Attempt2 compiled but its allocation oracle
misread the existing scan failure's global safe refusal as PID corruption; that
run did not reach the escaping collection allocation. Final attempt3 inspects
actual PID/archive/policy diagnostics, preserves all bytes, and actually reopens
before retry. Both earlier attempts remain pinned separately. Original owner
budgets remain300-second compilation and120-second aggregate execution. Final
before compile/runtime29.218/3.204 seconds; after SQL29.307/3.253; flat29.178/3.845.
Supplied wrong-SHA/backend/source guards pass in both after modes.

Component declaration `tmp/journal-deferral-qualified-artifact.local.json`,
SHA-256005585718299b20e19fdf46770dde91340550edaa2b6e57514d1592a604d3f67,
pins the unchanged19-case owner,520 production/header inputs plus two owners,
binary/log/result metadata and all1232 production inputs. SQL-header binary
88a4e5cd9365eab5efb79eab51f7d73179bedbb16ce17e8b90506c56cd4691f9;
flatfiledc7cfda2d063ae288b4764632dd9c80ac8c3f49732b455085558fe954744f9a9.
The component makes no SQL connection and does not establish database effects.

Both strict production builds pass original600-second budgets with-j2 and all
1232 native source identities unchanged. SQL completes in111 seconds including
the original pfile-only object target, with50 observed changed-header consumers;
flatfile96 seconds/49 consumers. SQL production binary
e6f166b6800ed64eaf58d3182fb2ceac36a4d18526c6f6705b035a5b9fa4f4b9;
flatfileb338612a985d6618dd1921371f11434293163432c4bdbceff7ea20839c2c172d.
Existing worker, pipeline, journal, quarantine, phase01 recovery, diagnostic,
54 writer,2 evidence and7 root-harness owners pass.14 accounting validations and
30 accounting tests also pass on this frozen source, recorded by the tool output
(session18916/chunk197518), without a separate captured log. Actual inactive SQL
elapsed/quiet/repeated saves, link loss, quit/restart, death/reload, crash recovery
and live copyover pass on fresh disposable MariaDB and MySQL instances, followed
by the maintained actual reconciliation/allocation and spell-receipt owners.
Both owned schema/server teardowns and port-rebind checks pass. Actual flatfile
creation/save/coldrestart/relog passes in131.804 seconds. These journeys exercise
normal inactive callbacks, not restored-save deferral with actual SQL effects.

Final nonmutating clang-format18 fixpoints, generated matrix and whitespace
checks pass. Immutable `tmp/journal-deferral-final-milestone.local.json`, SHA-256
5b86b898e006a32b6c9df94c48c00a5ccee050ce18a527db7d1df27a1818a9dd,
reverifies1232 native/current source pins, all component artifacts/case logs,
both strict build binaries/logs, gameplay/teardown, maintained owners and the
unchanged false/BLOCKED matrix. The captured-tool-only14/30 and format evidence
are labeled separately. This bounded journal prerequisite is solved locally;
normal GitHub publication still awaits local history integration authorization.

This remains a journal-only prerequisite. It does not establish a resident hold
or permission to hydrate a deferred player. The durable critical owner must
restore its gate before cold replay; a later wake must be retained. Startup
census, checkpoint-spanning permits/generation recheck, independent worker ACK
and public checkpoint fences, stale-frame disposition and original-publication
ACK reservation remain unwired. The later milestone below closes bounded callback bad_alloc classification and
proof withdrawal on unresolved retry/ambiguity. General postcallback allocation,
SQL transaction/pool-lease cleanup and worker queue allocation gaps remain open.
No route, R1-R8 or release gate is promoted; coverage_complete=false/release=BLOCKED.

### Unresolved callback failures must retain earlier same-PID proofs

The next paired native owner establishes two separate failure paths. Callback
bad_alloc was caught as terminal EFAULT and quarantined the entire affected PID;
returned retry/ambiguity retained earlier ordinary durable99 or exact death/
quest/spell/craft proofs, so stop_replay checkpointing retired earlier frames
despite the unresolved later request. All19 unresolved cases fail on the actual
before source, while four genuine terminal controls pass. Strict SQL-header
before compilation takes28.172 seconds/300; aggregate execution1.604/120. Binary
4f6cbb73911b4eaea96d2ce6e773a3e44e93db1bebd1b2b42104c7c6c5503ac8;
`tmp/journal-unresolved-before-qualified-artifact-v2.local.json` SHA-256
391d84aa9a19b32aa0595a430476d4549cfe23e3fa2b46f4d9cea511193e1533.

The candidate catches only std::bad_alloc before the existing catch-all, records
retryable ENOMEM with zero durability proof, and withdraws both proof stores
before stopping on any retryable or ambiguous outcome. Unaffected prior PIDs
checkpoint; later callbacks remain unattempted and the global replay/load fence
stays closed. Real checkpoint errors keep precedence. Shared allocation-free
proof withdrawal also serves existing deferral/quarantine without changing their
meaning. Runtime EFAULT, explicit terminal ENOMEM, custody error10001/diagnosis6
and death error10001/diagnosis11 retain genuine durable quarantine semantics.
Read-only review found no source blocker. This does not repair ordinary SQL
transaction or pool-lease cleanup, possible-COMMIT readback ownership, or general
postcallback allocation safety.

The final23-case owner includes intended callback branch counters and exact
native save_replay_result PID/revision/outcome/error/diagnosis/durable0 assertions;
frame retention alone would not establish error classification. Complete raw
headers/payload/duplicate IDs, earlier exact proofs, unaffected earlier and
unattempted later PIDs, repeated replay, actual cold reopen and real fdatasync
failure/repair are checked. The marked-effect exception is explicitly a controlled
uncertainty simulation, not SQL COMMIT/rollback/lease proof. Native SQL-header and
flatfile23-case after runs pass, along with fresh unchanged19-case deferral
regressions per mode on the same new source. Component declaration
`tmp/journal-unresolved-qualified-artifact.local.json` SHA-256
2ba29acd58b51f9c2a2c027b504b0743e105e1b50f78c4faddf38c07e9a139de
pins all five before/after artifacts and every case log, full compiler argv,
source/header/owner identities and three supplied negative guards per run.
After23 SQL compile/runtime32.821/1.810 seconds; flat27.873/1.561. Refreshed19
SQL28.707/3.366; flat28.939/3.637. Original300/120 gates remain unchanged.

Both incremental strict production builds pass original600-second budgets/-j2,
recompiling the one changed journal translation unit and retaining unchanged
objects under the identical source closure. SQL18 seconds, binary SHA-256
b5ab9b49afb341a91cc0915ba1038b69925205013824bd99fd5ccea179a8c513;
flatfile15 seconds,cc8909496f1536c631c41b01f37948a0745c928af67f6e3ab1eb88c0c6ffe57a.
All nine maintained worker/pipeline/journal/quarantine/recovery/diagnostic/writer/
evidence/root owners pass.14 accounting validations,30 contracts and matrix
--check pass with captured logs, retaining868/2817/2758/zero unmapped and false/
BLOCKED. Actual inactive SQL gameplay plus maintained native follow-ups pass
on fresh MariaDB and MySQL. Actual inactive flatfile creation/save/cold restart/
relog passes in129.215 seconds within the unchanged600-second outer budget.
Owned SQL schemas were torn down, exact disposable server processes stopped and
ports35281/35282 rebound successfully. Final nonmutating format/matrix/whitespace
checks pass. Immutable `tmp/journal-unresolved-final-milestone.local.json`, SHA-256
9ef8c4b2071c26b0c95209266a3dc32d48306caeeb93a604fa652e6efc581dfe,
reverifies all1232 native source pins, five before/after component declarations,
case logs/binaries, builds, gameplay, teardown and maintained owner logs. This
bounded journal prerequisite is solved locally. No R1-R8 gate is promoted.

The initial candidate preparation stopped on one clang-format aggregate line
wrap before any compiler, object forcing or manifest write. Its original source
2eeac6700fd81968162183ae2aa6fd243f2d9d1b69e90d82dceb647e208c106b
remains preserved. The formatted frozen candidate journal source is
f1a1f7bd5f8abb9cdc1cd663edc8db8b003bb8f080ecae095bea794ac3522272;
all other1231 native inputs remain unchanged. Production source manifest
`tmp/journal-unresolved-production-inputs.local.json`, SHA-256
d530541c8950ae5d1f8ee69f707ed7eb23bfb5e5ca48b2703d0febf1da152874,
pins that failed-preparation witness and the new1232-input source copy. Its
strict builds recompiled the only changed journal translation unit, retaining
unchanged objects under the prior identical source closure. The private final
verifier initially ran under Windows and refused Linux paths before any evidence
write; its subsequent Linux run verified the original pinned evidence.

Source-only review also confirms a separate typed worker identity gap: initial
submission/pending promotion can narrow components after the original journal
append, so real ACK re-encoding conflicts with the retained frame. Typed-required
bits can also be removed. Separate revision ownership and original apply/ACK
identity must preserve native custody/stale-frame safety; flatfile currently lacks
SQL apply-time custody comparison. This remains unimplemented. General worker
allocation, SQL transaction/lease cleanup, restored-save gates, current broad and
combined incoming-source qualification remain open. GitHub publication remains
pending local history integration authorization under the no-merge restriction.

### Retained worker admission: reproduced failures; repair in preparation

The frozen19-case admission owner links actual worker/revision/codec/journal/
observability with a controlled canonical apply callback and real journal ACK.
The first actual SQL-header run reproduces15 admission failures: allocation can
consume caller input before make_unique, leave a slot after queue failure, or
narrow the retained input after a real older ACK. No/real-journal initial ordinals,
pending/replacement/protected-deferral and measured native ready-set/deque growth
all fail exact caller/owner checks. Cumulative receipt coalescing, PID capacity
and null-pointer controls pass. The byte-capacity control has a separate fixture
setup failure: capture remembered8192, then the final serialized bound changed
to4MiB without resealing the oracle. Its seven controlled apply refusals are not
counted as production evidence. The original executed owner/artifact is retained:
`tmp/worker-admission-before-qualified-artifact-v2.local.json` SHA-256
d65caa6eeb4a4b6b38526273762d60e16e37fde4b6fcf28a687901298fef4cde;
binary e71122c0e09c15d3e0e1b1df046579f1380b418bce1ffe27da50656fb43e13eb,
strict compile37.410 seconds within300. The revised owner will repair that final
oracle and add a genuine newer-uncaptured-mark refusal/repair case, preserving
original capacity, assertions and real ACK gates.

Read-only review confirms a related replacement admission bug: fail_inflight
regenerates queued identity from current state. A newer uncaptured mark after
r2 capture can make begin_inflight fail after the original active revision and
caller input were changed. Validate current revision/unacknowledged mask before
replacement, and stage empty job/slot/cancelable ready ownership before any move,
mask change or revision claim. The proposed private source candidate has not
been applied or qualified yet. Typed mask/exact-journal identity, retry/promotion
and result-delivery allocation are separate issues. No gate is promoted.

The corrected frozen20-case owner establishes16 production semantic failures and
four controls PASS on the unchanged original source. Strict compile45.192/300,
aggregate6.324/120; binary b9c0850316d5d302affd7aef8f6dd5d6279223d84f8bb1a53397f2f15ae48b53.
The new uncaptured-mark witness records input_exact0/revision_exact0 under the
actual old replacement path. Final-byte-capacity resealing restores that genuine
control without changing any production capacity or native assertion. Revised
runner1317d86a05d7e17702a01de26cb924b066d260cfe59c149e69060bbb13f10b17,
harness e93ddfff47761ab6c479d655a7aede1df10629d1a5d53e1ae067d6063a3276b1;
`tmp/worker-admission-before-qualified-artifact-v3.local.json` SHA-256
e119a981a8cae55c76f6b831c5faeaf31c6f86ceb837559a7eae3b24a1c6e51c.

The reviewed candidate now allocates an empty job, empty PID slot and cancelable
ready ownership before claiming revision state or moving the caller capture.
All commit steps after begin_inflight are nonallocating, including a statically
noexcept snapshot move. Refusal removes staged owners without altering retained
bytes/input/revision. Pending/replacement allocation also precedes moving input;
undispatched replacement validates current revision/unacknowledged components
before fail_inflight can regenerate queued identity. Existing return enums,
capacity limits, inactive selection and typed mask behavior are preserved.

All20 after cases pass per SQL-header and flatfile mode, along with unchanged8
worker-parking regressions per mode rebuilt on this exact source. Admission SQL
compile/runtime38.221/6.401 seconds, flat37.971/5.502; parking42.900/0.685 and
35.648/0.579. Original300/120 gates are unchanged. Immutable component declaration
`tmp/worker-admission-qualified-artifact.local.json` SHA-256
fd0e36bf355708722f2d4d4a6edfd52b391fa794a2bf0fb4a5cc882a704a45a8
pins five actual artifacts, binaries/compiler argv/header+owner inputs, native
case logs/results and negative wrong-SHA/backend/source guards. It also retains
the first byte-capacity fixture failure. Driver preparation first refused a
wrong prior-manifest path, then a missing parking-owner path, both before any
compiler/precompile declaration; original scripts are preserved. The recovered
driver uses the verified original frozen parking owner, with assertions intact.
Frozen worker source101bb4cbc60060e139efce7de63a79886af4e13323c94a59f47d745a4e3e5ef8;
all other1231 native inputs unchanged, production manifest
`tmp/worker-admission-production-inputs.local.json` SHA-256
ff3ec51aebda8baf055701c2a185512e01cee6d99d05b424fce12c6376f0db11.
Both strict incremental builds pass original600/-j2, forcing exactly the changed
worker translation unit per backend;13 seconds each. SQL binary
c83bed3969dc22eae46d5979c41f858ade75dd8d09d592b5c868c3ee04b90798;
flatfile b8a985130fb7919bc2f701caafafd641351d9a2dc99b46620ee62d6d33847925.
All nine maintained owners,14 accounting validations/30 contracts and matrix
--check pass. Actual inactive MariaDB b13f3fc7 and MySQL1fd7e752 journeys pass
elapsed/quiet/repeated saves, link loss, quit/restart, death/reload, crash recovery,
live copyover and maintained native follow-ups. Flatfile creation/save/cold
restart/relog passes131.800 seconds within its original600-second outer budget.
Final owned schema/server teardown and port35283/35284 rebind checks pass.
Immutable `tmp/worker-admission-final-milestone.local.json`, SHA-256
20a99f56dc10e0efa0a701cf25563a8e594ca6bb9c791fe694d7c75314fc3882,
reverifies all1232 native/current source pins, five component declarations/case
logs/binaries, strict build outputs, gameplay/teardown, maintained owners and
unchanged false/BLOCKED matrix. Nonmutating clang-format18 and final whitespace
checks pass. This bounded retained-admission issue is solved locally. Typed mask
identity, retry/promotion/result delivery, ordinary SQL cleanup, restored-save
gates and full/current/combined-source acceptance remain open. No route is promoted.

A fresh read-only remote fetch is now3dbb8bc831a3caf2ae37381e844ce203e89e9379,
12 local/114 incoming commits. The eight new incoming commits merge PR597 finite
renewable spell wards, extending affects/snapshot wire identity and adding0056.
Worker admission and ordinary SQL cleanup remain unfixed in that incoming source.
Preserve new ward semantics and earlier durable-only death-descendant evidence
on eventual authorized integration. Local qualification remains source/schema
0055 only; combined incoming/0056 checks are pending. No history integration or
push was performed under the retained no-merge restriction.

### Retained worker retry and pending promotion: bounded issue qualified locally

The frozen15-case scheduling owner links actual worker/revision/codec/journal/
observability with controlled apply and real journal ACK. Its first strict native
compile failed on an int-to-unsigned conditional in the fixture; no behavioral
case ran. Original owner, compile argv/attempt and log remain preserved in
`tmp/worker-scheduling-before-qualified-artifact-v1.local.json`, SHA-256
`d7e1d9cd78ad59b4593d8dc2b753f0afcb9ea10431ed4aae4bd779fd7aa6ff4f`.
The correction changes the fixture error-code type only and retains every oracle.

Qualified BEFORE declaration
`tmp/worker-scheduling-before-qualified-artifact-v2.local.json`, SHA-256
`6f700fb4981da8f840eb9089c0ebd07328e2959d4f8a0ddd3ac40666864b3d06`,
records11 semantic failures/four controls. The40.022-second native compile is
within its original300-second budget; aggregate cases remain within120 seconds.
Actual native ready-set node/rehash and deque-block growth allocations reproduce
escaping exceptions after the deliverable result or revision/receipt ownership
has changed, including typed quest/spell/craft completions after real journal ACK.

The repair stages cancelable readiness under the worker mutex before consuming
results or changing health, retries, revisions or receipt ownership. Failed queue
allocation returns the already completed prefix and retains the original front
for a later pulse. Existing-entry scheduling explicitly avoids allocation;
completion moves are statically checked nothrow. Failed pending promotion cancels
only newly staged readiness. No typed component-mask or SQL behavior changes.
The scheduling15 cases, unchanged20-case admission and8-case parking regressions
pass on both backend builds (86 AFTER cases overall). Seven immutable artifacts,
case logs, compiler argv and wrong-SHA/backend/source guards are pinned by
`tmp/worker-scheduling-qualified-artifact.local.json`, SHA-256
`36ec899671943bc65aad8ccec12bca0ed6bc1c975cf2c6655459137756c4a160`.
Native compiles remain within300 seconds and each aggregate runtime within120;
no gate was expanded. Both strict incremental server builds pass with the changed
worker translation unit forced, in16 SQL/9 flatfile seconds. Nine maintained
owners,14 validations/30 contracts and nonmutating coverage matrix pass.

Actual inactive MariaDB13523e02 and MySQLa7367c8c journeys pass elapsed/quiet/
repeated saves, linkloss, quit/restart, death/reload, crash recovery and live
copyover. Actual native item reconciliation/partial-forest allocation and
spell/quest receipt follow-ups also pass. Flatfile creation/save/coldrestart/relog
passes133.956 seconds within the unchanged600-second wrapper. Final declaration
`tmp/worker-scheduling-final-milestone.local.json`, SHA-256
`1c6b2a43a50d08adbfb87e27328202d3c97116a6d553536da5c52da1754ddcf6`,
reverifies all source/artifact/binary/log/case pins, clang-format18, owned SQL
schema/server teardown and port rebind. This bounded scheduling issue is solved
locally; no active accounting or integrated restored-save behavior is claimed.

Only worker.c changes among the1,232 frozen production inputs. Current manifest
`tmp/worker-scheduling-production-inputs.local.json` has SHA-256
`2576febfc2c3a1de1eec6052d79fe0104c99b67b5cacf9e0ed5404509496e692`.
This source/schema0055 milestone excludes incoming0056 integration and does not
qualify result-queue push allocation, typed exact journal identity, ordinary SQL
transaction/lease cleanup, restored-save production wiring or full R1-R8.

### Ordinary SQL exception cleanup owner: prepared, not executed

The independent proposed owner now has13 cases linking eight actual production
units: repository/codec/journal, extra-description codec, observability, death
conflict, SQL lifecycle guard and pool. It covers real START/DML allocation,
real COMMIT with hidden reply and replacement-readback allocation, failed original
ROLLBACK, consumed-null replacement, borrowed transaction/autocommit/reconnect
refusal, custody-conflict continuation and separate malloc-buffer/MYSQL_RES faults.
This is preparation only; source transaction/lease/resource cleanup is not fixed.

Read-only fixture review caught a unique-name seed collision, an early description
failure that prevented the pet test, missing actual reconnect-option readback and
a failed-cleanup oracle that could accept stale durable proof. Deterministic PID
names, independent resource cases, option readback and exact genuine custody
failure/witness plus below-request revision checks correct those setup/oracle
gaps. Draft snapshots and preparation receipts remain preserved; no native
failure is inferred from a static review or previous draft.

Frozen runner SHA-256
`b1da8dbe2369749c3ece9b6e234d19fa995663039f52cc2e008cf9840b38e863`;
harness SHA-256
`2af04b1d2490e6e43614788a2724fb98883973892eda265322efc6294b4d8ae1`.
`tmp/player-snapshot-exception-prepared-artifact-v3.local.json`, SHA-256
`605e7d71a800409f7a87c73e289393fd1c1494e32acf69acd1441528828fee38`,
records527 inputs, AST/clang-format18 and14 negative disposable-target guards.
Proposed300-second compile/120-second aggregate bounds remain unmeasured and do
not relabel older600-second compilation gates. Native compile and actual private
MySQL/MariaDB cases are pending. Synthetic craft entitlement rows exercise the
repository seam rather than producer provenance; controlled ROLLBACK failure does
not establish successful cleanup. Worker/pipeline, producer/coordinator, general
allocator sweep, death evidence transaction sweep and incoming0056 are excluded.

### Ordinary SQL cleanup: actual paired failure evidence, implementation pending

The original strict fixture compile failed before behavior on a signed PID
initializer; its argv, frozen owner and log remain in declaration93421178. A
type-only correction preserved all oracles. The first executed owner produced
12 genuine failures/one consumed-null replacement control on each actual engine
(declaration27d1a049). Diagnostic-only refinement then measured the borrowed and
conflict states without changing those13 oracles. Current runner SHA-256
`b1da8dbe2369749c3ece9b6e234d19fa995663039f52cc2e008cf9840b38e863`;
current harness SHA-256
`ae1bbc75e8a79db2c549ffd3c34824a0c11b86292cc27047881083dfa70b4d42`.

The refined native compile passes51.774 seconds within its original300-second
bound. Actual private MariaDBf4e0d7b1 and MySQL1dd038ec each attempt all13 cases
and reproduce12 failures/one control within the original120-second aggregate
bound. START/DML allocation escapes without rollback; pooled paths retain a
lease. Actual COMMIT followed by hidden reply/replacement readback allocation
leaves the replacement lease held. Failed original rollback returns an unclosed
unsafe session to the pool. Borrowed transaction, autocommit-OFF and reconnect-ON
connections actually return applied, durable revision2, and changed rows; the
first two also change caller transaction settings. Description buffers and pet
MYSQL_RES leak at measured allocation seams. These are post-apply failures, not
seed, codec, target-guard or setup errors.

Failed custody-conflict cleanup begins only the original transaction: starts1,
rollback1/confirmed0, rows unchanged. The existing idle gate prevents a second
evidence transaction. The returned EBUSY/empty diagnosis loses original custody
error10001, diagnosis12 and witness, while the unsafe original session is released
available. Preserve that distinction; no second evidence START was observed.

Qualified BEFORE declaration
`tmp/player-snapshot-exception-before-qualified-artifact-v3.local.json`, SHA-256
`8e986daf4182e7e40f3ce589847e41a9b0eb7a4ef2e7419540a87a7d908ee2da`,
verifies1,232 native/current source inputs,527 component inputs, owner/binary/log/
case pins, three wrong-artifact guards, canonical private0055 migrations,
idempotence/accounting fingerprints and owned teardown/port rebind. Both engines
use the same native client ABI. Retry/healthy controls after a failing assertion
remain AFTER obligations. Source cleanup remains unmodified; no requirement or
acceptance gate is promoted. Incoming0056, producer provenance, worker/pipeline,
general allocator and retained evidence sweeps remain outside this proof.

### Completion delivery: first native failure reproduction and calibration gap

The initial12-case native owner compiles36.941 seconds within300 seconds. Four
quest/spell/craft/unrelated block-growth cases prove a worker-thread bad_alloc
abort after successful real exact journal ACK and independent raw-frame removal.
Seven retry, real ACK-repair, healthy, reopen,384/448-delivery wrap and full256
shutdown controls pass. The mixed-map case is unqualified: its main-thread pulse
can drain retained fillers before target enqueue, changing actual deque growth
from the probe. No calibrated fault marker was observed for that case. Preserve
this result as a fixture ordering gap; it is not a fifth semantic failure.

First declaration `tmp/worker-result-delivery-before-qualified-artifact-v1.local.json`,
SHA-256 `460905b98527205deab2a172f15e4d17e24b0de3e4cdeb8f871f8bd55bb1d243`,
retains binary16088883, all12 observations, four full abort-proof witnesses, source/
owner/artifact pins and guards. Production source remains unchanged. Corrected
immutable native calibration and the private fixed-capacity queue candidate still
require qualification; no issue, route or R1-R8 gate is completed by this attempt.

Corrected completion calibration preserves the real no-pop prefix until the
actual native enqueue signal. Full12 BEFORE rerun now records five independently
proven worker-thread post-ACK allocation aborts and seven passing controls; no
oracle or300/120 budget changed. Native compile39.762 seconds, declaration
`tmp/worker-result-delivery-before-qualified-artifact-v2.local.json`, SHA-256
`a34138e3a8e1a93b3d5043b71a77686ab60b26ca84af4b19794804fc40073f1e`,
pins binary7d68a54c and all source/owner/log/raw-frame identities. The original
mixed-map attempt460905b9 remains preserved and unqualified. Corrected harness
SHA-256 `4690e4ce8fea28a9df4f84eb3a716e4b9cfc6fb2c88682522f591f74d450b774`;
runner remainsd0b58a71.

The reviewed production candidate replaces the allocating completion deque with
a256-entry fixed FIFO under the same mutex/bounds, nothrow move constraints and
existing readiness/receipt ownership. Static capacity relates results to the
maximum distinct active PIDs. The full-results shutdown skip is not a reproduced
bug: distinct dispatched slots bound pre-enqueue depth below the result limit.
The immutable candidate manifesta92afb73 contains1,232 inputs with only worker.c
changed from scheduling9ba7020a. Paired AFTER/build/gameplay qualification is
running; this remains an implementation candidate, not a solved issue.

SQL candidate source review found and corrected a double mysql_errno read in
failed-rollback proof; a nonzero ROLLBACK result must always remain unconfirmed.
The borrowing contract now explicitly says save success does not prove session
reuse and cleanup-aware callers inspect every return. No further new source
blocker was found. Four independent retained-participant allocation/failed-cleanup/
uncertain-commit cases are required beyond SQL13 and the maintained terminal
matrix, including explicit cleanup-before-writer-fence-release observation.
Lifecycle lock acquisition/release sustained-allocation failure and general
recovery_apply remain separate limits; no SQL source candidate is integrated yet.

The first completion AFTER owner compiles36.377 seconds and passes11/12 cases.
The remaining spell case delivered the target allocation-free but failed its
unrelated-PID check: that predicate combined in-flight and queued counts from
two independently locked health snapshots. Dispatch between them can fabricate
a settled state. One consistent snapshot corrects the fixture, with the exact
healthy-PID completion oracle retained. Original owner4690e4ce and artifact
`tmp/worker-result-delivery-regression-delivery-after-sql-v1-artifact.local.json`
SHA-256 `0ca4c9f411f04d73049b0f2ce252f8db51a36afa00935cc47da9524dcad2e869`
remain preserved. Corrected owneraaa77dc6 requires a full paired rerun. Neither
that first AFTER attempt nor any isolated target pass completes this milestone.

The consistent-health owneraaa77dc6 is requalified against immutable original
scheduling source9ba7020a:37.497-second compile, five proven post-ACK aborts/seven
controls, aggregate12.126 seconds. Declaration
`tmp/worker-result-delivery-before-qualified-artifact-v3.local.json`, SHA-256
`2f81786677a307dc9341bf977502e21fd08ecf98549391b528b6f0a0ed4530c4`,
explicitly verifies the sole current worktree delta is the reviewed ring worker;
the original BEFORE native source and other1,231 current inputs remain unchanged.
All12 corrected delivery AFTER cases pass both modes, including zero post-ACK
allocations and exact FIFO/receipts/capacity/reopen controls. Remaining unchanged
worker owners, strict builds and actual gameplay/recovery remain milestone gates.

All110 completion-candidate AFTER cases now pass:12 delivery/15 scheduling/20
admission/eight parking per mode. Paired component declaration
`tmp/worker-result-delivery-qualified-artifact.local.json`, SHA-256
`7146a76567cecac7a377641366fad957c7b662830038be368e413ab0a9a82e3b`,
verifies nine frozen artifacts, all guards/source/owner/log/case pins, original
300/120 bounds and both preserved fixture attempts. Both forced strict native
server builds pass in15 SQL/11 flatfile seconds within unchanged600/-j2 limits.
Only worker.c changes among1,232 frozen production inputs. Maintained owners,
accounting contracts and actual inactive gameplay/recovery remain pending; no
completion milestone or R1-R8 acceptance gate is declared solved yet.

The nine maintained worker/pipeline/journal/quarantine/recovery/diagnostics/writer/
evidence/root owners pass on the ring candidate. Accounting validation,30
contracts and nonmutating matrix check pass; coverage remainsfalse/releaseBLOCKED.
Actual SQL and flatfile gameplay/recovery are still pending, so no ring milestone
commit is made yet. An independently reviewed four-case retained SQL owner is
compiling against the unchanged SQL repositories; original13-case owner stays
unchanged. Native SQL qualification requires exact rollback/fence/readback/row/
journal witnesses, not an assumption that every BEFORE case must fail.

Both SQL qualification owners require explicit CLI artifacts/targets. On eventual
SQL integration register their distinct names in the regression runner's existing
manual-only list, as for the other supplied-schema native owners; the generic
runner invokes discovered scripts without arguments. This avoids treating CLI
argument errors as regressions or silently treating a missing SQL run as proof.

### Retained SQL participant: independent paired BEFORE qualification

Distinct four-case runnerb5dd0b55/harnessc25081cc passes48.957-second native
compile against unchanged SQL repositories. An early stale temporary-directory
prefix guard refused before initialization; a subsequent attempt exposed the
107-byte Unix socket limit and stopped both owned engines before migrations or
cases. Setup receipts and roots/logs are preserved. A short fresh root prefix and
explicit socket-length check repair setup only; schema settings, source, binaries,
oracles and original300/120 bounds are unchanged.

Actual private MariaDB9b1aef1e and MySQL6bb9bc18 attempt all four cases. Post-START
work allocation and allocation after real evidence INSERT are valid BEFORE
controls: both original-session rollbacks succeed before the first writer-lock
release, exact retry settles and healthy PID progresses. These do not establish
throwing-constructor coverage. Failed second rollback genuinely returns an
unclosed unsafe original session available; actual COMMIT followed by distinct
replacement exact-record readback allocation genuinely returns terminal ENOMEM
instead of retaining ambiguity. Each engine has two controls/two genuine failures.
Both query APIs are observed without double counting; first fence-release attempt
and actual original sessions/SQL boundaries are recorded allocation-free.

Declaration `tmp/player-death-conflict-exception-before-qualified-artifact-v1.local.json`,
SHA-256 `a5b3de94f7e7063dee01e0abb08da34d1ce73582b999e0225ef168c5dd4a7491`,
verifies1,232 native/current-LF source inputs,527 component/owner inputs, binary
e9afdae3, all case/log/report pins, three wrong-artifact guards, private0055
migrations/idempotence/fingerprints, preserved setup attempts and owned engine
teardown/port rebind. One native client ABI is used on both servers. Empty other
receipt/inbox/outbox projections are unintended-write controls, not populated
receipt preservation or producer provenance. Failed cases leave later AFTER
assertions pending. Lifecycle sustained-allocation/recovery_apply/incoming0056
remain outside this bounded proof. Source implementation is still isolated; no
SQL issue or R1-R8 gate is marked solved.

### Completion delivery: qualified local milestone

Worker completion delivery after a real exact journal ACK is now solved locally.
The allocating result deque is replaced by a fixed 256-entry FIFO under the same
mutex, capacity/backpressure and receipt ownership. Nothrow moves preserve
completion delivery when allocation is unavailable after the journal frame is
removed. Five proven BEFORE aborts/seven controls become twelve AFTER passes per
backend, including FIFO wrap, partial/full capacity, shutdown/reopen, exact typed
receipts, real ACK failure/repair and unrelated-PID progress. Unchanged scheduling,
admission and parking owners contribute another 86 AFTER passes (110 total).
Both strict incremental server builds, nine maintained owners,14 validations/30
contracts and nonmutating matrix pass. Actual inactive MariaDB/MySQL save, death,
crash and copyover journeys plus native follow-ups pass; flatfile creation/save/
cold restart/relog passes. Final declaration edd75c103ad576d8d8b0c994696a0108b09914e0533725db536a8cfeec4d9678 verifies all1,232 production
inputs, component artifacts, binaries/logs and owned SQL teardown/port rebind.
Preserved fixture failures remain evidence, not production failures. Ordinary SQL
cleanup, typed exact journal identity, restored-save integration, incoming0056 and
broad qualification remain open. No R1-R8 gate or coverage status is promoted.

The final immutable receipt is `tmp/worker-result-delivery-final-milestone.local.json`,
SHA-256 `edd75c103ad576d8d8b0c994696a0108b09914e0533725db536a8cfeec4d9678`. Worker sourceab105ee1, runnerd0b58a71 and harnessaaa77dc6 retain
the unchanged300-second compile/120-second aggregate component bounds. The strict
builds force changed worker.o in both modes within600 seconds/-j2; they are
incremental server evidence, not cold/tool or combined-remote qualification.
Actual engine runs use private0055 schemas and one native client ABI. SQL wallet
root item exclusion, inactive accounting and the declined inactive spell paths
are preserved. The original full-results shutdown skip is not a reproduced bug.
Normal GitHub publication still requires permitted history integration under the
retained no-merge restriction; no force push, merge, deployment or activation.

### SQL cleanup candidate integration and native qualification

The reviewed five-file SQL cleanup candidate is integrated after committed worker
completion milestone dfc879598. Borrowed idle/autocommit/reconnect-disabled
sessions report original-session cleanup proof; pooled owners retire unconfirmed
connections, preserve consumed replacement ownership and ambiguous COMMIT, and
retain original custody evidence when rollback is unconfirmed. Separate retained
transaction cleanup precedes writer-fence release. Native buffer/result ownership
is scoped by RAII. Recovery_apply and sustained lifecycle lock allocation remain
separate work.

Manifest `tmp/sql-cleanup-production-inputs.local.json`, SHA-256
`efcb65d1faa86b6bd95eac6c576cf3b336033d26a01d0a16b5f46b475425de52`,
freezes1,233 source inputs: four changed repository source/headers and one new
SQL-only helper. All actual dependency-recorded header consumers are forced:
47 SQL/46 flatfile objects. Source preparation first refused Windows path
separator mismatch, then src-relative dependency parsing; both failed setup
attempts are archived before source/native object mutation. Corrected setup
matches exact dependency tokens and retains every consumer check.

Unchanged ordinary13 and retained4 owners are registered as explicit manual SQL
probes. Frozen AFTER drivers preserve original300/120 bounds, canonical0055
schema settings, all source/binary/owner/case pins, three rejection guards and
owned teardown/identity/port rebind. Strict builds and native AFTER qualification
are pending. Source integration is not a solved SQL issue or R1-R8 acceptance.

Both strict source-pinned server builds now pass on SQL cleanup candidate.
First SQL make passed but its complete-consumer declaration correctly refused
an unbuilt affected `core/files.pf.o`, which the server target does not require.
The first log/build and refusal are preserved. An explicit object-target follow-up
compiles that remaining consumer in4 seconds; combined compile logs prove all47
SQL consumers. Flatfile rebuilds all46 explicit consumers and links in95 seconds.
Both phases retain600-second/-j2 limits. This is incremental server and affected
object evidence, not full pfile-tool binaries/cold or economic route qualification.
SQL strict binaryd072d52d and flat43b701c8 remain pinned to source manifestefcb65d1.
Ordinary13/retained4 ASan/UBSan owners are
compiling serially with unchanged300/120 bounds; actual AFTER cases and gameplay
remain pending. No SQL issue or R1-R8 gate is declared solved.

### Follow-up recovery and lifecycle ownership gaps (source review only)

Recovery must cover the whole borrowing/proof/lease boundary, not only
`player_snapshot_repository_recovery_apply`. Its cleanup is installed after
START, ignores rollback and lacks reconnect-disabled/original-session checks.
`player_quarantine_recovery.c::sql_proof` can accept an otherwise valid row proof
after unconfirmed rollback; journal recovery_resolve can then remove quarantine.
`prepare_sql` shares unchecked cleanup. Boot revalidate_selected chooses pool
reuse by mysql_errno rather than demonstrated original-session cleanup.
Use the SQL-specific cleanup/lease primitives through resume and proof, preserving
all archive/backend-generation/creation/UID/component/state comparisons. Refuse
successful verification when cleanup is unconfirmed; revision alone is not proof.
Native reproduction is pending: actual START reply loss, DML allocation/rollback,
proof allocation plus persistent rollback refusal, valid proof with both rollback
attempts refused, real pooled boot revalidation and ambiguous COMMIT/exact retry.
Exact preparation/archive/journal/SQL rows, quarantine fence and lease witnesses
are required. No ordinary post-successful-COMMIT allocation seam was found; do
not manufacture such a BEFORE claim.

Separate lifecycle work is required in economic_sql_lifecycle_guard.c. Lock and
unlock allocate SQL strings inside uncaught noexcept acquisitions/destructors.
Acquisition publishes local authority before SQL ownership is transferred; a
possibly successful GET_LOCK needs tentative original-session ownership. Unlock
ignores verified release before clearing local exclusion/coordinator ownership.
Use bounded stack SQL/allocation-free parsing and checked same-session release,
keeping SQL outside authority mutexes and SQL/local/coordinator release order.
Uncertain release must retain exclusion and support retry; catch-and-ignore is
insufficient. Native sustained-allocation, partial acquisition, lost lock reply,
failed release, independent lock ownership and unrelated-admission proofs remain
pending. These source findings are separate milestones; current frozen SQL cleanup
candidate and0055 source pins remain unchanged. No schema or R1-R8 gate promotion.

Ordinary and retained SQL AFTER components now pass all34 cases (13+4 per engine)
on fresh MariaDB40d0d2a4/MySQL private fixtures. Unchanged native owners compile
with ASan/UBSan in60.049/51.430 seconds within300 seconds; each runtime retains
its120-second aggregate limit. Declaration
`tmp/sql-cleanup-after-components-qualified-v1.local.json`, SHA-256
`cb02c167d395b83c30b9971713a396fd6de8a659d9e3aad9b4baa9de406d40b8`,
verifies1,233 source entries/528 inputs per owner, binaries, three rejection
guards, actual original-session rollback/retirement/ambiguity/resource oracles,
private canonical0055 migrations/idempotence/fingerprints and owned schema/process
teardown/port rebind. Paired BEFORE remains12 failures/one control ordinary and
two failures/two controls retained per engine. This closes component proof only;
maintained terminal matrix, normal inactive gameplay and recovery remain required
before the SQL milestone is solved or committed. Full accounting remains blocked.

The first maintained worker owner passes its actual keyed runtime, then fails an
old source-text requirement that `sql_pool_replace_connection` appear directly
in repository.c. That call is now in the consuming lease helper, as exercised by
the unchanged real-pool13 owner on both engines. Original owner66c33da4 and the
first failure log are archived. The repaired owner follows immediate pool lease
acquisition, its replacement call and clearing old ownership before consuming
replacement, while retaining all worker runtime/revision/commit checks. An initial
repair preparation refused an assumed consumer spelling before editing the owner;
the exact `lease(sql_pool_acquire())` spelling is checked instead. Native worker
and pipeline maintained reruns pass; the remaining eleven maintained owners are
running. Original SQL17 owners/source/binaries and paired declaration stay unchanged.

### SQL cleanup public-header regression and corrected qualification

The revised maintained gate passes worker/pipeline/journal/quarantine/recovery,
then diagnostics exposes a real dependency regression: public recovery headers
transitively include the SQL pool helper, whose `<mysql.h>` needs native SQL flags.
The unchanged generic staff diagnostic command intentionally has only `-Isrc`;
adding SQL flags would conceal this coupling. A read-only architect confirms all
public cleanup uses are pointers and concrete cleanup objects occur only in the
two repository implementations/helper.

Both public repository headers now forward-declare player_sql_cleanup; both .c
implementations explicitly include the private helper. Existing mysql/mysql.h,
overload signatures/noexcept and the helper's enum/struct/RAII behavior remain
unchanged. No extra metadata header is added (production inventory stays1,233).
The first candidate source/34 passing actual SQL cases remain immutable evidence
for their original closure, not the corrected source. Header repair preparation
first applied the snapshot pair before refusing mixed-CRLF conflict matching;
second preparation refused the already applied pair. Complete four-file targets
were then checked against pinned original/corrected LF states before completion.
The original failure and integration receipt are preserved.

Corrected-source maintained checks, all affected header-consumer strict builds,
unchanged17 cases per real engine, maintained terminal matrix and actual inactive
gameplay/recovery must pass before this SQL issue is declared solved. No accounting
activation, wallet-root item inclusion, declined spell-path retry or R1-R8 promotion.

Corrected header closure is now frozen separately under production manifest
`1eb6bd0e5c5f1705ada372968b9426aec4bd951bb8596c401e58612acb1d5512`.
SQL strict production rebuild passes in114 seconds with all47 affected consumers
(including files.pf.o) actually compiled; flatfile is running serially with its
original600-second/-j2 gate. Worker/pipeline/journal/quarantine/recovery/diagnostics
maintained owners pass at corrected source. Writer coverage passes54 checks, then
route evidence catches a stale census coordinate: the route was2298 but the saved
census retained2295. Both now name the identical statement at2298; only these
existing coordinates change. The original failure and two exact location-repair
receipts remain archived. Matrix regeneration preserves868 routes/2817 occurrences/
2758 unique sites, coveragefalse/releaseBLOCKED; no semantic/evidence/source_commit
promotion. Remaining maintained checks and corrected-source real engine/terminal/
gameplay qualification are pending. Original source34 AFTER cases remain evidence
for their original closure only. Recovery proof/lease, lifecycle, typed identity,
restored wiring and full R1-R8 remain separate unfinished work.

Both corrected-source strict builds pass: SQL47 affected consumers/114 seconds,
flatfile46/101 seconds, unchanged600-second/-j2 limits and1,233-input source closure.
Unchanged ordinary/retained native owners compile in61.960/53.383 seconds with
ASan/UBSan and all three source/backend/binary rejection guards. All34 actual
AFTER cases pass on MariaDB1ab90aaa/MySQLda855883. The corrected-source paired
component declaration is
`tmp/sql-cleanup-header-after-components-qualified-v1.local.json`, SHA-256
`a8007791cdb1e0c59a79b223daff34ee84fc1784a2cff4b8764706f51afeac2f`.
It verifies original BEFORE failures, all current/frozen source, owner, binary,
case/log/support pins, exact owned schema DROP, process identity/absence and port
rebind. The frozen17-case driver proves schema removal by successful exact DROP;
it does not record a separate absence SELECT. Original first-source34 AFTER
artifactcb02c167 remains unchanged and excluded as corrected-source proof.
Maintained terminal owner compiles in63.599 seconds within600; its fresh-schema
matrix is running. Ordinary inactive gameplay and final qualification remain open.

A fresh read-only remote fetch now resolves experimental-accounting to
`b84693f9620b33f86ca0261ff07bb7b93f966774`:15 local/123 incoming commits.
Nine additional incoming commits since3dbb8bc83 add exact-UID payload repair,
accounting/custody guards, transaction visibility wait and Dispel Magic duration
behavior. They remain outside the local canonical0055 qualification closure.
Local history integration remains blocked by the retained no-merge instruction
and automatic approval review; no merge/rebase/cherry-pick/divergent push attempted.
No accounting gate or combined remote qualification is promoted.

Read-only review of3dbb8bc83..b84693f96 confirms no additional migration: incoming
head remains0056. None of the nine commits touches worker/journal/snapshot/death-
conflict/quarantine-recovery cleanup seams. Preserve new offline exact-UID repair
classification payload_repair/disposition30, journal/conflict/reward/ancestry/coin/
custody refusals and strict conversion/SQL-warning checks. Preserve incoming loader
runtime companion affect slots and description/spellbook fidelity, shared metadata
validation/codec build dependencies, centralized Dispel object policies and portal
pointer lifetime correction. Incoming writer inventory has not classified new/moved
repair/Dispel sites; refresh discovery and route classification only after authorized
integration. Current local zero-unmapped counts and0055 results exclude that delta.
No declined vines/faerie-sight change or wallet-root policy is retried.

Corrected-source writer route evidence (2 tests), writer site contract (2,727 checks;
2,762 mapped including retained inventory scopes) and writer coverage (54 tests)
now pass after the census coordinate repair. These contract counts remain distinct
from the current lexical2,758 unique source sites and executable route evidence.
All five maintained terminal groups pass on MariaDB:9 native invocations with5
fresh canonical schemas, including default cold restart/concurrent replay and basic
terminal restart. Typed quest/spell groups exercise internal replay, not typed cold
restart. Whole owned schema drops plus group absence-count0, identity/process stop
and port rebind pass. MySQL matrix is running; full inactive gameplay and final
milestone qualification still pending. No SQL issue or full R1-R8 gate is solved yet.


### Corrected SQL cleanup regression qualification and integration authorization

All14 maintained owners now pass on the corrected header closure: worker,
pipeline, journal, quarantined dispatcher, phase01 recovery, diagnostics, writer
route/site/coverage contracts, root harness, real death selector, evidence codec,
journal lifecycle and cold-load fence. Root discovery first refused a stale exact
manual-only set after the two explicitly supplied SQL artifact owners were added;
only those two expected names change. Original failure/owner and exact repair
receipt remain archived; discovery exclusion, serialized-resource, timeout,
cancellation and signal tests remain intact (seven behavioral cases pass).

Maintained terminal qualification passes on both actual engines:18 native command
invocations in10 fresh canonical0055 schema groups. The independent declaration
`tmp/sql-cleanup-header-maintained-terminal-qualified-v1.local.json` is SHA-256
`ff2115b6d5840614f9dfdab2e4f3c72f91b0fdca28be02f7940d8b89d53b9d03`.
It binds527 native/source/owner inputs, sanitizer binary, original600-second
compile/group limits, exact group DROP and explicit schema absence, owned process
identity/stop and port rebind. Typed quest/spell internal replay remains distinct
from typed cold restart; this owner uses controlled pool stubs. Current14 contract
fixtures,30 accounting tests and nonmutating matrix check also pass. Seven changed
C/C++ source/header/owner files pass nonmutating clang-format18 fixpoints.
Corrected-source actual inactive gameplay and final milestone declaration remain
pending; these component results do not solve full accounting qualification.

The user explicitly approved the previously blocked local Git merge to integrate
new experimental-accounting commits. Preserve this authorization separately from
GitHub PR merge/deploy/production activation, which remain prohibited. Complete
and commit the bounded SQL cleanup issue first; then refresh and integrate both
histories locally, qualify the combined source, and push normally. Previously
recorded rejection and source-specific qualification remain historical evidence;
no unauthorized history mutation or divergent push was attempted.


### Bounded ordinary/retained SQL cleanup solved locally

Final declaration `tmp/sql-cleanup-header-final-milestone-v1.local.json`, SHA-256
`0a0b5c5a4e683119db303a478c92dea956060a87a85d9782b1ba20e353865aaf`,
rechecks all1,233 current/frozen production inputs, paired BEFORE and corrected
AFTER artifact/binary/owner/log/support pins, strict builds,14 maintained owners,
14 accounting fixtures/30 contracts, matrix and seven format fixpoints. Both
actual engines pass13 ordinary+4 retained fault cases (34 total); the separate
maintained terminal matrix passes18 native invocations/10 fresh schema groups.
Original300/120 fault-owner and600 terminal/build budgets remain unchanged.

The current strict SQL server passes real inactive save/death/crash/livecopyover
journeys plus actual item/partial-forest/OOM/spell/quest persistence follow-ups on
MariaDB7c1b354b (195.605 seconds) and MySQL35ef7379 (297.333), within1200 each.
Journey-owned schema drops leave no wrapper leftovers; independent baseline
schema equality/absence, owned PID/session/executable/datadir stop and port rebind
pass. Current strict flatfile creation/save/coldrestart/relog passes132.529 seconds
within600. Accounting scanner/contract checks ran on Windows; native compilers,
owners/builds and gameplay ran in WSL. A final declaration first refused a raw
Windows log path in WSL; exact scoped alias mapping corrected the verifier without
rerunning or weakening qualification. Original prepared versions/refusal remain
preserved. This bounded transaction/lease/resource cleanup issue is solved locally.

Pooled owners retire unconfirmed original-session cleanup and preserve replacement
ownership; borrowed callers retain responsibility for their handles. Custody
conflict diagnostics survive failed rollback and the separate retained transaction
is cleaned before writer-fence release. Ambiguous COMMIT remains ambiguous.
Recovery preparation/proof/lease, lifecycle release/exclusion, typed exact journal
identity and restored-save production wiring remain separate unfinished work.
Canonical0055 results exclude incoming0056 until integration and requalification.
Coverage remains false/releaseBLOCKED; no R1-R8 or activation gate is promoted.

The user now assigns Plans1-4/shared coordinator/contracts/producers/registry/
matrix/activation to this primary stream. A separate user-coordinated agent owns
Plan5 independent reconciliation/audit/backup-restore/release qualification. Return
narrow interface requests and independently committed slices for integration on
one tested candidate; do not duplicate Plan5 mutation/tooling work. Local Git
integration is explicitly authorized; GitHub PR merge/deploy/production activation
remain prohibited. Normal milestone publication follows combined-source checks.


### Current combined-source integration checkpoint

Local merge `03438beab` combines qualified SQL-cleanup parent `79e766b54` with
incoming `9d0ea2a1` and canonical migration 0056. All seven conflicts are resolved
and committed. Local merge `d19459bd` also integrates Plan 5's `7d2f8e815` orphan
audit slice. Wider candidate qualification and normal publication remain pending.
Local definite-admission disposition is retained after the incoming diagnostic-only
recovery correlation field. ACK reservation and failure retention are unchanged.
The incoming manifest-based runner is preserved and eleven local owners are
explicitly registered; the two mandatory-artifact SQL owners remain manual.

Registry reconciliation preserves both branches' semantic changes and reanchors
checked excerpt identities rather than selecting one side's coordinates. Current
inventory is 872 routes / 2,818 lexical occurrences / 2,759 unique sites, with zero
unmapped lexical sites. Four source-only rows classify offline player payload
repair and dispel object, portal and anchor mutations beyond the lexical scan.
All four remain backend-unverified; coverage remains false and release BLOCKED.
The refreshed activation contract passes 55 cases, writer evidence passes two,
writer-site checks pass 2,728 checks, and accounting contracts pass 30 cases.
Source-coordinate assertion failures and the Windows python3 alias failure are
retained before the exact checked anchor/interpreter repairs.

`tmp/accounting-integration-9d0-native-owners-v1.local.json` pins the combined native
inputs and nine passing serial owners: coordinator, spell wards/dispel, world
activity runtime/contract, nevent scheduler, latency, save journal, pipeline and
worker. Diagnostics passes separately with the supported linker wrapper; its
original missing-system-libm setup failure is retained. These are native component
checks, not complete SQL, player, ward restart or release qualification.
Runtime compatibility validation passes for 0056. Linux migration/boot contracts
pass all 24 and 10 cases respectively after Windows ownership/symlink/socket/
interpreter boundaries; original failures remain retained. Combined-source Plan 5
unit checks pass 63 reconciliation, 11 audit-origin and 16 audit-invariant cases.
Reduced-schema native audit evidence still requires current-candidate rerun and
does not establish canonical fresh/upgrade or interruption/CLI qualification.
The already-underway retention group completed all ten native runs successfully:
four admission/visibility/checkpoint/capacity owners plus coin, admission-owner
release and craft retention in SQL-header and flatfile modes. Each craft mode
passes all 30 scenarios. `tmp/accounting-integration-retention-v1.local.json`
retains the exact source/owner/log hashes; process completion and all ten results
are confirmed. These controlled component checks do not qualify actual SQL or
player/restart routes. No new test run was started after the batching request.

Two isolated implementation specialists now own shop publication retention and
REMOVE-CURSE custody fixtures/producer work. Primary owns their shared contract
dependencies and integration. Source preparation is concurrent; native compiler
and heavy runtime slots are bounded. See `SHARED_STREAM_HANDOFF.md` for the three
qualification scope. Subsequent user steering defers new test runs until each major
plan is ready. The check already underway may finish; queued new owners are canceled.
Immutable BEFORE sources and regression fixtures remain preserved for milestone
failure/fix qualification. Implementation commits remain unqualified until those
batches pass. No required R1-R8 gate is removed, and no
measured overall time saving is claimed yet. Plan 2's acceptance wording now matches
permanently deprecated active blackjack: refusal before wager/wallet/pending-payout
mutation, with existing inactive legacy coverage retained.

The root-runner integration tests refuse the missing required matrix rows for the
ordinary and retained SQL exception owners. Preserve workload completeness and
supply real compile/run adapters with the original 300/120 budgets; a target guard
self-test cannot qualify the native cases. The narrow Plan 5 request and first
orphan-audit review are in `SHARED_STREAM_HANDOFF.md`. Strict combined-source server
builds, paired native SQL faults, active route/player/recovery qualification and
current broad gates remain open. Earlier canonical 0055 receipts remain scoped to
their original source; no production activation/data change or PR merge is authorized.

### Source implementation after plan-level testing deferral

The user-specified repository remains `Community-Duris/Duris`. The shared Git
`origin` now points to `Community-Duris/DurisMUD`, which has no accounting branch;
that configuration is preserved. Direct authority-scoped fetch confirms requested
experimental-accounting remains `7d2f8e815`, while Plan 5 advances to `f2a110c4d`.
The retained UID provenance query slice is merged locally, followed by the
`0a13bde70` REMOVE-CURSE preparation helper. The helper is unwired, pointer-free
and read-only; active refusal and the existing spell source remain unchanged.
Both slices require current-candidate plan-level qualification.

The primary implements recovery cleanup across snapshot recovery apply,
preparation, exact proof and boot pooled-session revalidation. Borrowed APIs retain
their original signatures through wrappers and add explicit cleanup overloads;
they require idle autocommit with reconnect disabled and never close or replace
the handle. Cleanup is armed before START, including the inspection loader's
second read transaction. Exact original-session rollback must be confirmed before
preparation or journal resolution. COMMIT-attempt tracking preserves ambiguity.
Boot leases retire by default, and later session retirement does not falsely undo
successful durable recovery resolution. Archive/generation, original creation,
UID, component and full projection checks remain intact; no migration is added.

The read-only persistence review caught the second inspection transaction and
post-resolution return-contract gaps; both are corrected in source. Immutable
BEFORE archive `tmp/recovery-session-cleanup-before-v3.local/manifest.json` is
SHA-256 `7338380f04afb42269c97a45a49fce9c8fb9e1698918c5fb3b3d8a7056554abb`
at base `002cb6013dd7e5a14fa18e69467a4db31f1f6e8c`. This is implemented,
unqualified work: no native RED, compiler, SQL or gameplay test was run. Deferred
Plan 1 fixtures must retain the existing 13 fault cases and add second-phase
inspection START loss/OOM/rollback refusal plus late retirement after resolution.
The old 0055 owner and qualification remain historical. Plan 1, writer evidence,
activation and release gates remain open.

### Sealed worker identity implementation; Plan 1 tests deferred

The worker now keeps a receipt-bearing or death snapshot's original component
mask after journal append. A separate internal claimed mask records the narrower
revision obligation after an older exact ACK. Initial admission, pending
promotion, undispatched replacement and retry/failure use the claim for revision
bookkeeping; repository application, journal ACK and public completion retain the
sealed original body/mask. Source review of the production death/literal pipeline
requires that public-mask distinction. A later capture must cover the active
claim, rather than redundantly captured already-ACKed bits. Replacement validates
and claims the exact current queued mask, including a full-body death capture. Ordinary saves keep their existing narrowed-mask behavior.
No receipt, death body, revision or public completion interface is invented.

The source failure is the old assignment to `snapshot.components` at admission
and promotion after append: the canonical typed journal ACK compares the encoded
original body and refuses the changed mask. Immutable BEFORE manifest
`tmp/worker-sealed-identity-before-v2.local/manifest.json` is SHA-256
`d8d661504f138955954edd6156e22f388682be8578e182f6ecda82e171c2a604`,
base `541f938499a7919bbc3ed3397c58076a6bf927a5`. Regression fixture preparation
is independent; no compiler/native/SQL/gameplay qualification is run. This source
implementation remains unqualified until the Plan 1 failure/fix batch.

Shop retention slice `672389002` is integrated locally: stable operation ownership,
canonical receipt retention, staged physical publication and started/returned
handler fences. Its 37 prepared cases per SQL-header/flat profile are unexecuted.
Schema-2 accounting admission, complete literal payload and cold-replay ownership
remain shared dependencies. Recovery V3 has 18 unexecuted SQL cases, retaining the
original 300-second compile/120-second aggregate runtime limits; preparation
receipt `tmp/sql-recovery-cleanup-owner-prepared-v3.local.json` is SHA-256
`386a9a75f93b58370c336863ec351b6911fdb6a5948ca3f24204af28274cc5a2`.
No route, R1-R8, activation or release gate is promoted by source integration.

### Lifecycle owner and cleanup propagation implementation; unqualified

Acquisition now binds the original SQL session, acquiring thread and local
exclusion before issuing GET_LOCK. A separate confirmation bit grants authority
only after native readback and lifecycle-state validation. Same-session borrowed
named locks are refused before acquisition. Checked release validates the owner
thread before SQL or mutex operations, proves original-session nonownership, and
retains uncertainty. Each cleanup call permits one RELEASE_LOCK attempt. Failed
RPCs can retry on a later owner call only after fresh original-session readback
proves the fence remains held. A successful RPC still showing ownership remains
fenced and cannot drain recursive leases. Maintained native pre-send failure and
partial-release recovery cases remain required and unchanged.

The coordinator release callback now reports exact lease/thread success. Cutover
transfer and publication handoff preserve the thread and confirmed authority;
partial SQL cleanup retains the remaining obligation. Runtime acquisition failure
keeps an unresolved connection and guard together in a non-ready owner; shutdown
can retry cleanup, and forked children still avoid inherited protocol traffic.

Critical-command and death-conflict pool adapters retain writer guards through
transaction cleanup. A new pool operation validates an exact currently borrowed
slot, closes it, then allows local writer exclusion to end. Foreign/direct DB
handles are untouched. The pooled player lease checks the same guard/handle
identity and clears the consumed pointer. Unresolved borrowed writer destruction
latches runtime SQL exclusion closed and retains local exclusion for process
recovery; it never silently closes a borrowed DB handle. Known committed results,
including exact retained replay after independently confirmed rollback, survive a
later named-lock cleanup failure. Ambiguous outcomes retain original-ID recovery.

Read-only review caught foreign-thread cleanup ordering and a retained replay
result downgrade; both are corrected in source. BEFORE manifest
`tmp/lifecycle-owned-release-before-v2.local/manifest.json` is SHA-256
`0460595a3b4b810a5864d85bb42b32995aa7e67aac4a0930e8941a9a732d8855`,
base `7c391d98d`; the supplementary pool header is separately Git-normalized.
New pool/guard link symbols require maintained fixture-double updates. Fault-owner
preparation remains unexecuted; original budgets and both actual SQL engines remain
required. This source implementation has no compiler, native, SQL or gameplay
qualification. Plan 1 also still needs restored-save production wiring and the
complete integrated failure/replay/restart batch; no acceptance gate is promoted.

Worker identity preparation V2 has 25 unexecuted cases with real journal append,
canonical frame and ACK checks, controlled repository outcomes and ordinary
controls. Its receipt `tmp/worker-typed-identity-prepared-v2.local.json` is SHA-256
`d7f22f403cff50d2a72471ebeefdbeac86f421da7fc7f5bb49e7aa04338b4eaa`,
binding 529 inputs at `7c391d98d`. Original 300/120 limits are unmeasured.


### Phased save startup prerequisite; source only

Save pipeline preparation and execution are now separate opt-in APIs. Preparation
opens validated journal/recovery metadata with admission and load replay closed,
without starting persistence workers or the dispatcher. Start preserves restored
metadata, refuses duplicate execution and leaves prepared holds intact on startup
failure; cleanup/join happens outside the pipeline mutex. Resume and the synchronous
save/hydration admission helpers cannot reopen a prepared, unstarted pipeline.
The existing init wrapper retains immediate prepare/start behavior and tears down
on failed start. Production comm boot calls remain unchanged until the complete
restored-save ownership handoff is implemented; this API does not close that race.

Read-only architecture review confirms the coordinator already restores all replay
observers before its workers launch. Durable-save census must scan the actual
journal, including retained/quarantined frames; resident pipeline diagnostics are
not that census. Checkpoint-spanning apply permits, original operation/generation
holds, independent ACK fences, retained wakes, actorless native hydration and
partial-startup dependent-owner cleanup remain required before production wiring.

BEFORE manifest tmp/save-startup-phase-before-v1.local/manifest.json is SHA-256
07923782e8b4dc477e971b44bd8e3b0aea795d366e563c22bdcfd11e96bcdebb,
base65e4f1590872eaf6514663568b7b36bbdb1b73a8. Deferred qualification must cover
prepared admission and shutdown, duplicate start, worker/hook/dispatcher failure,
retry retaining exact holds, legacy inactive initialization and the full overlapping
save/critical recovery journey. No compiler/native/SQL/gameplay tests ran.


### Ordinary room coin producer slice integrated; unqualified

Local merge af900753 contains independent implementation6b6b7c10c and the
optional post-ACK staging release contract96e2af83a. Active ordinary-room single-root
coin drop and pickup now use a retained native publication adapter. Original UID,
canonical before/after literal bytes, denominations, custody and exact result
revisions must agree. Native materialization, amount updates and placement keep
explicit started/returned states; uncertain effects remain held. Every ACK retry
revalidates physical evidence and the current wallet body. Work is one attempt per
pulse; notifications and bulk continuation follow durable ACK and owner extraction.
Inactive/schema1 paths remain separate, unsupported active placements refuse and
callback-free cold replay stays held. Production Makefile registration is primary-owned.

Final BEFORE /opt/duris-accounting-coin-publication-before-final-18fbd004fc3e/source
manifest SHA-25648792b5f103e286305ec1c4282b0e864ef494cd8d7566347311b2a41aa2235c7
pins1238 files. Candidate /opt/duris-accounting-coin-publication-committed-3457636af08c/source
manifest45432924e1c0b872d1f9eb3b4916ac93fc033706dfd74089fc145af6a10860ba pins1240.
Prepared24physical+15owner cases per SQL-header/flat profile are unexecuted; BEFORE
uses --scope owner, AFTER --scope all. Their native capture/codec/runtime custody
plus controlled placement/render/materializer seams do not qualify actual actobj
handlers, either database or flatfile recovery. Original300-second compile and
30-second per-case bounds remain unmeasured. Four initial review findings were
corrected; primary final-pin source review remains separate from qualification.

Open: semantic writer/central owner registration, actual native producer and
backend journeys, cold routing, actorless hydration and explicit uncertain native
effect recovery. No source/fixture inventory is promoted to R1-R8 or route evidence.


Validated active-journal observation prerequisite (source only): the new
collect_retained API collects all validated active frames under the journal mutex,
including exact original bytes/record identities and current quarantine/policy
flags. A temporary result is published only after complete collection; allocation
or scan failure returns an empty output without apply/checkpoint. Existing
corruption scanning may archive evidence and latch the global fence. This is an
observation that expires after unlocking, not an apply/checkpoint/ACK permit,
hydration proof or complete archive census. Record ID alone is not exact identity.
The inherited scanner treats a missing file as empty; future durable-absence
claims must independently establish file/generation authority.

BEFORE tmp/save-journal-census-before-v1.local/manifest.json is SHA-256
75d98c2b4ef1a2d5b43c4ff0baac2ece6a3eea4ab2a43072e6c2367b3d6dac58,
base38432876b. Read-only source review found no bounded-API blocker. Deferred
cases: exact bytes/duplicate identities, allocation failure, quarantine/policy
flags, mixed valid/corrupt records, missing-file and concurrent-mutation behavior.
No compiler, native, SQL, gameplay or recovery checks ran. No production caller
uses this prerequisite yet; the full ownership protocol remains open.

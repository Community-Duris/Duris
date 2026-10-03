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

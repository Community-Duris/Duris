# Plan 5 original retention-fixture link qualification — 2026-10-11

ACC-06-RETENTION-LINK-01 establishes the actual original linker defect and
qualifies its exact three-entry fixture repair. The unchanged primary SQL
fixture fails with 11 distinct undefined symbols from three omitted definition
providers. Adding those three real source files makes both original SQL and
flatfile fixture builds and runtimes pass under ASan/UBSan. Independent decoding
passes, and the emitted bytes agree exactly across both builds.

Qualification is **PASS_LINK_STRUCTURAL_COMPONENT_ONLY**. The fixture contains
two zero-effect roots; full retention, deletion, claims, financial producers,
schema acceptance, gameplay, native authority and release remain unqualified.

## Source, publication and ownership

| Item | Exact identity |
| --- | --- |
| Owned branch and remote destination | `codex/accounting-plan5` |
| Worktree | `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max` |
| Preparation base → published preparation | `bea3c44ee0dcb34fffa166eee9d51dcfdd27a49d` → `3b863dba809c7064b096b7e293bda98c70cc0ed1` |
| Terminal report base | `3b863dba809c7064b096b7e293bda98c70cc0ed1` |
| Frozen tested primary | `0555741d740cf028552fcf63973265bfbcda052c` |
| Frozen source tree | `36ff31fdd214ca80c69b5edf2c4aabcc1f69b2ef` |
| Preparation publication receipt | `delivery/preparation-publication.json`, SHA `4c5645a4ab4bed3f3e125e1a6466b0afd5e13e4c78d719c81ba226d7fbdd052c` |
| Actual native session | `36030`, terminal exit **0** |

The terminal report/result commit and verified remote SHA are bound after
publication in `delivery/result.json`. Owned maintained changes are this report
and additive [remote follow-up](PLAN5_REMOTE_FOLLOWUP_2026-10-06.md). The exact
[three-provider patch](patches/PLAN5_RETENTION_FIXTURE_THREE_PROVIDERS_2026-10-11.patch)
is already published with the preparation. There are no shared interface/schema
requests or changes to runtime, accounting contracts, migrations, writer
registry/matrix or activation ownership.

The broad existing Plan5 retention module is preserved. It differs from current
primary in claims/64/full-journey work and must not be imported wholesale for
this ticket. The tested candidate is a separate byte-exact primary copy with
only three source-list entries added. The coordinator should apply the exact
patch to its matching primary-derived fixture, preserving unrelated work.
Primary adoption and notebook import require their separate acknowledgment.

## Actual defect and complete bounded fix

Original module: `tests/async/run_plan5_retention_journeys.py`, Git blob
`915826708723c8d0bc11d5c64bf6228ea114527a`, SHA
`52846af18be2d42e43c28509104d225a42c5ee3a67da3b1d23e8c3bf0399e91f`.
Its original `native_fixture()` source list has 11 translation units. The
unchanged `item_transfer_command.c` consumer now references definitions in:

```text
src/item/lockpick_retirement_continuation.c
src/economy/native_quest_cost.c
src/economy/native_quest_coin_give.c
```

The actual original SQL build exits 1 with `collect2` linker failure. Its
11 distinct undefined symbols are the lockpick validity pair, four quest-cost
encode/decode variants, and five quest-coin projection/encode/decode variants.
All belong to those three real providers. Complete 9,305-byte diagnostics are
retained, SHA `de6ff87c3831d47755d839991f69c8f94fcdfce2297a4bc8b1cd45db95be8bae`.
The original module's `check=True` aborts at its first SQL build; original flat
is **NOT_RUN_AFTER_SQL_LINK_FAILURE**. No flat original failure is inferred.

Only this actual red outcome permits the candidate's 14-TU source list. Candidate
module SHA `3dc38861cdb14f6aae61f586a0943c38962fa9a17c93b40615480b106a042878`;
patch SHA `798921bab0d5ace50a01c3cb5afecad027c53223e68ba31951d5f03848a9e6ed`.
Removing precisely the three entries restores the entire original Python AST.
Original generated C++, flags, assertions, other functions and profile
assertions are unchanged. The generated 1,917-byte C++ source in both build
roots has SHA `0434ec834bd8b76e80280e39fa0f243b503168f8c517f33413fc903e5b7d10b7`.
No mock/stub, function patch, migration-head substitution or assertion waiver.

## Commands, backends and results

Host invocation, executed once with its distinct reviewed resource grant:

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B D:/Dev/Tests/Duris/accounting-plan5/ACC-06-RETENTION-LINK-01/helpers/run_retention_link.py --preparation D:/Dev/Tests/Duris/accounting-plan5/ACC-06-RETENTION-LINK-01/preparation-receipt.json --grant D:/Dev/Temp/accounting-acceleration-bootstrap-20261011/ACC-06-RETENTION-LINK-01-resource-grant.json
```

Inside the foreground QA container, `python3 -u -B /helpers/retention_link_driver.py`
executes the real module's `native_fixture()` through these recorded commands:

```text
timeout --signal=TERM --kill-after=5s 180s /usr/bin/python3 -u -B /helpers/retention_link_driver.py --build original
timeout --signal=TERM --kill-after=5s 360s /usr/bin/python3 -u -B /helpers/retention_link_driver.py --build candidate
/opt/candidate/bin/tests/plan5-retention-canonical-native/probe-flatfile 7
/opt/candidate/bin/tests/plan5-retention-canonical-native/probe-sql 7
```

The unchanged recipe uses GCC 13.3.0, C++20, `-Wall -Wextra -Wpedantic -Werror
-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie
-Isrc`, the original generated probe and source list, and `-lcrypto`. Flatfile
adds the original `-D__NO_MYSQL__`. The original SQL failure records the exact
full compiler argv. Runtime preserves `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`
and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`, with 20-second bounds.

| Actual check | Result |
| --- | --- |
| Original SQL compile/link | Exit 1, expected three-provider linker family |
| Original flatfile compile/link | NOT_RUN after original SQL failure |
| Candidate SQL and flatfile compile/link | Both pass; combined build exit 0 |
| Candidate SQL and flatfile PID7 runtimes | Both exit 0; stderr empty; native assertions pass |
| Original emitted framing | Four EAI1/EAP1 blocks per backend, two root pairs |
| Independent frozen decoder | Digest bindings, domain digest, lineage/epoch, writer/policy/compiler/reason, domain actor/PID7, operation/original chain and all six zero counts pass |
| SQL/flat bytes | Identical 1,040-byte streams, SHA `908254540ef02aad6f815f467deece34b98cc672726de54c19fb071575422963` |
| Native / Docker foreground / outer transport | All exit 0; foreground duration 412.031 seconds |
| Native/source retries | Zero |

Both 26,867,136-byte candidate binaries have SHA
`3c01a9bb7a0d9bc7e2d8d6064b603e8f2b182a81ed33d86c37bb797bd0d710c5`.
This fixture does not exercise database engine behavior. No MySQL/MariaDB,
server, full production build, gameplay or disposable-database check is run;
those require the coherent primary route and a separate grant. No C++ source
changes require a new maintained full build for this test-recipe-only repair.

## Evidence seal, resources and cleanup

Evidence root: `D:/Dev/Tests/Duris/accounting-plan5/ACC-06-RETENTION-LINK-01`.
Preparation/source/import proofs and all original/candidate diagnostics and
frames are retained. All 1,427 frozen source files, candidate/patch and pinned
helpers remain unchanged. The [preparation report](PLAN5_RETENTION_LINK_PREPARATION_2026-10-11.md)
records original blob/source inventory/archive and exact helper/grant pins.

| Retained artifact | SHA256 |
| --- | --- |
| `terminal-seal.json` | `70e15b05d9f54ca0c29aca75b376fd0a68aed74af10c75074bfe13509ccc16cd` |
| `qualification-result.json` | `369b37693b1a2d5e33f292ddada22b1314c4aae9fdcd74fe0b31758e29a52ec8` |
| `native/native-result.json` | `86be403ad6e69e0f9c98773f7294d21b54b05b7344377463d662101c695aa9e7` |
| `job-receipt.json` | `a260add0ebc2a98832cfeb1b18f215a2eebb24391edc5c7e33ad60ad422c6e01` |
| `native/candidate-build-result.json` | `50abf9b24679efcc5029b88be03ef1ef707aef068ec57a206d267b6a055554c3` |

The seal covers 1,472 evidence files / 72,752,094 bytes, plus separately pinned
D: build artifacts. The actual native session ended before the pinned sealer
ran; seal verification and matching lease checks precede guarded release.
`delivery/released-lease.json` records release; global heavy lock is absent.

QA identity: `plan5-acc06-retention-link01-8137e72653-qa`, full ID
`ac5ecdd3cd58246a2cedd4f45dd522997ae0589f3d3ba1eb38c56752c70742ee`, image
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`.
Actual cgroup limit 2,147,483,648 bytes / CPU2; network none, read-only root,
foreground `--rm`, restart no. Fresh isolated direct D: source/helper/candidate,
bin/scratch/evidence mounts match the reviewed grant. It ran from
11:34:58.955981Z to 11:41:55.677200Z on 2026-10-11; release is 11:42:14.804993Z.

The owned QA is removed and exact owner/job inventory is empty. All seven
unrelated live service IDs are preserved. All four older passive database
volumes are retained unchanged with zero consumers. No other owner's job,
database, game, image, volume, source, existing build folder or evidence is
stopped, moved or deleted. No unresolved resource or cleanup blocker remains.

The coordinator's independent terminal review passes, SHA
`f50e5dc5c0182d1c01a506df566ff3eb1bc5afbd8a7f4f95c0163e68aed29e61`,
at `D:/Dev/Tests/Duris/accounting-accel-integration/ACC-06-RETENTION-LINK-01-review/terminal-review.json`.
It rechecks all seal members and frozen inputs, actual original red, candidate
ELF binaries and C++, all eight exact mounts/permissions, retained frame digests
and metadata (including lifecycle source events), and fresh resource inventories.
Each backend stream contains four 256-byte blocks with their four-byte lengths.
This additional review does not broaden qualification or claim primary adoption.

## Remaining gates and curator handoff

This closes the fixture linking prerequisite on its exact tested source. The
full journey's original schema61 assertion remains unchanged and needs a
primary-owned coherent current-schema route; restore and native stake also
retain older head assertions. Original schema65 focused diagnostics and audit
budget evidence keep their own scopes and do not qualify the full matrix.

Full backup/restore/retention, durable erasure across restore surfaces, authentic
native authority, admission, recovery, player/gameplay, load/host and R1–R8 remain
OPEN. Accounting stays inactive/CLOSED, coverage incomplete, release BLOCKED and
full Plan5 goal ACTIVE. Wallet-root item exclusions and the declined inactive
spell-path behavior are preserved. No activation, deployment, PR merge,
production mutation or audit autocorrection.

This report, exact patch and additive follow-up form the curator packet for the
primary-local notebook and coordinator review. No primary integration, notebook
import, qualified contract-field update or combined release candidate is claimed.

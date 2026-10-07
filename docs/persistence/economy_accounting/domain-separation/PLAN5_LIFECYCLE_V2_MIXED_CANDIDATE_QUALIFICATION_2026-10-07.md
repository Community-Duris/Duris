# Lifecycle V2 reader: exact mixed-candidate component qualification — 2026-10-07

The whole unmodified maintained `tests/async/test_flatfile_lifecycle_v2.py`
passes on exact primary `506b54c9958652895b61f58367e25fb67bc9c552`
(reader integration `7a5eac466`, registration `506b54c99`). All65 coverage
cases, five envelope controls and original unchanged-authority assertions ran;
12 witnesses accept,53 refuse independently, and skips=0. Accepted witnesses
round-trip through this candidate's real native EAB codec and match the driver's
independent Python digest oracle. This is a newly executed mixed-candidate
component result, not reuse of an external Plan5 executable.

This finite assignment followed the primary coordinator's concrete publication.
Actual `get_goal` still returned BLOCKED with the full continuing objective intact.
No duplicate Goal, fabricated resumption or completion update occurred. R0-R14
remain closed and unchanged. The only repository change is this owned handoff;
primary independent authentication/review and any remote plan link remain pending.

## Candidate and complete export

Whole Git tree `c0f0501176ca66b6138683211daa14f2221e0368`, native tree
`833d3085815b396861ad18a77635412212381e4b`, migration tree
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`. Every6461 tracked entry,
256124812 body bytes, was exported from `git ls-tree -rz` and `git cat-file
--batch`, checking the Git blob hash and SHA256 of every body. The complete
deterministic tar preserves canonical regular0644/0755 Git executable modes and
literal symlink targets. No worktree WIP, environment file or untracked fixture
enters the candidate. Archive SHA256:
`599bfd38e61563ceade7bae6dd8020067e2c104b38bc089566a2949dda1ea1c1`.

After extraction, all6461 actual Linux bodies, sizes, regular/executable modes
and links authenticate before execution, after the maintained command and at
terminal completion. Host audit independently reopens every tar member and
compares all three source-authentication receipts to the exact Git manifest.
Copying outputs to NTFS does not qualify Linux permission/stat assertions.

| Input | Exact Git blob | SHA256 |
| --- | --- | --- |
| tests/async/test_flatfile_lifecycle_v2.py | 90ee6a829bc5a2e0d0e3e64c7404b3e79b10c070 | 4f0b789d9137468f55003459eb0ffe345a88004023ce0e3ab0779aa52537d65d |
| tests/async/flatfile_lifecycle_v2_fixture.cpp | d139ffed657514fd5d9f33e476f1f322ec319c90 | 116680163780e841daa11c2bb97823a07fd433b82900b4f6a99eb9cd21f4fbb3 |
| scripts/qualify_flatfile_economic_authority.h | 593272f3ab668e970c0f561deed60cb3ba11a232 | f3ac41ffbc026b38b8ed0ef227fab07217828a5c8a4733c01af9b4cf6dc02e52 |
| scripts/qualify_flatfile_economic_lifecycle.h | 8f82d0dc30f3e1cf55ef4e0f37484fe18dcef3e3 | 4731204d7adc5ae4c51fb269bb1aca00b19a72bf291366a35eaf6a985e90d51b |
| tests/async/test_flatfile_accounting_store.py (original provider list) | 2d1ad605fc946dd6a9fdcedce7ac0f0f5fb7273c | 590c4bfd62a5b0ef16391192f6fee99faa4a062dcfa9afba20861d1af4366954 |
| tests/async/test_flatfile_restore_lifecycle_receipts.py (original retained observations) | ad5c9cf29b79494c8b21721e9c612286224735d3 | cabb5b4b72aacc92480db49dba9ff0d9af8a76450f85daa71946a08acfb008eb |

## Actual execution, commands and original limits

Separate owned container `duris-domain-plan5-v2-506b54c9-ac24`, pinned image
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`,
GCC13.3.0 Ubuntu24.04.1. `/workspace` is the complete candidate on a read-only
source volume; a separate Linux volume supplies fresh `/owned/bin/tests` outputs.
Root filesystem is read-only, `/tmp` is isolated writable tmpfs, network=none.
No other owner's container was inspected or operated. No server, database,
migration, full Make or prior R0-R14/native private batch ran.

Exact maintained command, cwd `/workspace`:

```sh
python3 -u -B tests/async/test_flatfile_lifecycle_v2.py --native-source /workspace --artifacts /owned/bin/tests/lifecycle-v2/native
```

The driver's own ROOT and `--native-source` resolve to the same exact export.
The original600-second compile and45-second individual subprocess limits remain;
the registered900-second outer command limit also remains. An external Python
`sitecustomize.py` observer delegates each `subprocess.run` with its original
args/kwargs and returns its original result or exception. It records complete
argv, timeout, elapsed time, exit and captured stdout/stderr without changing
source, provider stubs, flags, assertions, results or bounds. Only observer
`PYTHONPATH` and bytecode suppression are added; sanitizer runtime options are
not changed. Every observer/controller body is sealed with the packet.

Exact compiler argv is retained in `worker-evidence/observer/step-001.json` and
`worker-evidence/RESULT.json`. Its shell rendering is:

```sh
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -g \
  -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie \
  -ffunction-sections -fdata-sections -Wl,--gc-sections -D__NO_MYSQL__ \
  -I/workspace/src -I/workspace/src/no_mysql \
  /workspace/tests/async/flatfile_lifecycle_v2_fixture.cpp \
  /workspace/src/economy/economic_baseline_codec.c \
  /workspace/src/economy/economic_baseline_adapter.c \
  /workspace/src/flatfile/flatfile_accounting_store.c \
  /workspace/src/flatfile/flatfile_authority_transaction.c \
  /workspace/src/flatfile/flatfile_store.c \
  /workspace/src/persistence/critical_command.c \
  /workspace/src/economy/currency_command.c \
  /workspace/src/item/item_transfer_command.c \
  /workspace/src/world/quest_mobile_native_reference.c \
  /workspace/src/item/craft_pouch_mutation.c \
  /workspace/src/combat/chaos_pouch_ledger.c \
  /workspace/src/player/player_snapshot_codec.c \
  /workspace/src/economy/shop_trade_recovery_manifest.c \
  /workspace/src/economy/economic_accounting_types.c \
  /workspace/src/economy/economic_accounting_plan.c \
  /workspace/src/economy/economic_source_event.c \
  /workspace/src/economy/economic_accounting_intent.c \
  /workspace/src/economy/economic_currency_adapter.c \
  /workspace/src/item/lockpick_retirement_continuation.c \
  /workspace/src/economy/native_quest_cost.c \
  /workspace/src/economy/native_quest_coin_give.c \
  -lcrypto -pthread -o /owned/bin/tests/lifecycle-v2/native/fixture
```

Actual compile exit0,52.965790 seconds; maintained command exit0,54.314082
seconds. All71 observed subprocesses complete: one build,65 coverage calls and
five frames. Coverage calls exit0 and return their expected accept/refuse result.
Frames1/2 exit0; frames0/3/4 intentionally exit1 with the exact original refusal
message. These three expected refusals are passing controls. Maximum individual
case duration0.091685 seconds; no timeout, failure, skip, repair or rerun occurred
in this owned execution. Final stdout is exactly:

```json
{"cases": 65, "accepted": 12, "envelopes": 5, "skips": 0}
```

Main stderr is empty. Every original case verifies retained bodies plus Linux
mode, link count, inode, size and mtime remain unchanged. The host audit compares
all actual witness/mapping bytes, reports, stdout/stderr, order, argv and limits
against the frozen driver's original generators and build recipe. This metadata
audit does not reexecute native cases. EAB1/2, historicalV1, empty/wallet-bank/
pile-onlyV2, sorted piles,3071 paired holdings, UINT64 UID/revisions, INT32 room/
denomination bounds, native revision100, malformed pair fields/counts/ordering/
reserved bytes, truncation, trailing bytes and unknown versions retain their
original coverage. Zero denominations establish parser behavior only.

Produced ELF:23704544 bytes, mode0700, SHA256
`492b9e1196a8e5c097f4603e4a9154a6099c8db076018d4e9a3c10de4e04efa4`.
Actual readelf header/program/dynamic and ldd outputs, compiler identity/search
paths, resolved g++/cc1plus/collect2/ld/as and linked runtime-library body pins
are retained. No external passing peer binary is used.

After the successful command,22 dependency-only `-M` probes preserve its source
list and original compiler flags, one source per probe; all exit0. They report
517 unique compiler input paths. Normalizing absolute `../` aliases authenticates
89 tracked candidate bodies and428 system inputs; the original worker record
counts86 direct-spelling tracked names and preserves the other three aliases.
`AUDIT.json` supplies all89 normalized Git/blob/mode/SHA256 pins without rewriting
the original receipt. This is compiler-reported dependency closure, not a second
native execution or a full server build.

## Separate original primary failure and proof limits

The frozen [primary integration report](https://github.com/Community-Duris/Duris/blob/506b54c9958652895b61f58367e25fb67bc9c552/docs/persistence/economy_accounting/PLAN5_LIFECYCLE_V2_PRIMARY_INTEGRATION_2026-10-07.md)
retains its original link exit1 after64.094 seconds: the WSL closure lacked
`/lib/x86_64-linux-gnu/libm.so.6` and `libmvec.so.1`. Zero runtime cases ran there.
The [registration report](https://github.com/Community-Duris/Duris/blob/506b54c9958652895b61f58367e25fb67bc9c552/docs/persistence/economy_accounting/PLAN5_LIFECYCLE_V2_REGISTRATION_2026-10-07.md)
preserves the required65/12/five/zero result contract and original budgets.
Exact bodies and a separately labeled failure-context receipt are retained under
`original-primary/`. The primary's raw failed-link RESULT/logs are not in this
export and are not locally authenticated here. The new Docker pass does not
rewrite that failure or establish repair of the primary WSL environment.

Source review confirms DURELR permits nativeV1/V2, rejects unknown nativeV3/V4,
and keeps qualifier-catalogue versions distinct. V2 coverage derives only from
retained EAB rows with the original wallet/bank digest, sorted native UID/room/
revision/denominations/source fingerprint. Production receipt code still checks
operation/lineage/epoch/actor/opening/boundary, command descriptor and retained
plan/link proof. Those full receipt paths are source-inspected; this driver
executes the coverage function and modeled envelope framing, not the complete
original native V2 receipt/installer journey. Existing historical suites were
not rerun or claimed by this assignment.

All following dispositions remain false: native lifecycle V2 encoder executed,
lifecycle install executed, current world recaptured, combined candidate
qualified, release complete. The five envelopes have modeled bodies. Native
EAB roundtrip is a real codec result, not native lifecycle installation proof.
Native genesis refusal of zero-value physical piles remains separate. Original
V2 encode/decode/install/retry/fault fixtures, full combined builds, census,
activation and release remain primary obligations; accounting stays inactive,
all926 writer policies and original Plans1-5/applicable R1-R8 gates remain.
SQL/full both-engine qualification is outside this flatfile component assignment
and remains required for the broader goal. No deployment or activation occurred.

## Retained packet and independent review handoff

Private packet: `bin/tests/domain-plan5-lifecycle-v2-506b54c9-20261007` in the
ac24 worktree. Absolute host root:
`C:/Users/alexa/.codex/worktrees/ac24/NewDuris Max/bin/tests/domain-plan5-lifecycle-v2-506b54c9-20261007`.
Its502-file flat artifact index binds292576776 bytes; the index excludes itself.
The separate original Linux seal binds466 files/73 directories and29615415 file
bytes before host copy. Every copied artifact body authenticates to that seal.
Source/evidence volumes and the exited owned worker are retained for independent
review. Authoritative terminal inspect confirms exited/exit0, no restart/OOM,
the pinned image, read-only source and network=none. No live job remains.

| Packet member | SHA256 |
| --- | --- |
| artifact-index.json (502 files) | d5afb7de2f931770cd67eb00fe0b7e2a6e57694a9f4b0fa3c088035d6efd0a5c |
| source-manifest.json | b29654755c311776017749abd42f688f7740bc0317d701be49e056a8cbdfad91 |
| worker-evidence/RESULT.json | 3a63a956e4faa16ec8ab4f3dc51fadc5b67c5e72cbf3b8aa2c76f463e35ff30f |
| worker-evidence/native/evidence.json | 14388fe0219ab3664b7f096f002aa7d372fd00210d1fd799856d27f6f2d0b47d |
| worker-evidence/compiler-dependencies.json | 8acf4988db0a0727ea7df70b886d23ac8b73e4b4094f259e0f7508bc554bdbfe |
| worker-evidence/linux-artifact-seal.json | 5b9fb41b4f5e5f5b3bc8ebf7b97eba0861db7ffb3f9fe7c9d3853543b444700a |
| AUDIT.json (normalized closure/case/recipe/copy audit) | 20780d6b8470dd63f38a03b6e7e344ad312c10b437a43b7d7134416101b70ff5 |
| host-RESULT.json (Docker argv/terminal/copy) | ecd4985a6b451c905ea21f447a919d687e79b1f458c39ddd2dacb005626af48e |
| post-RESULT.json (Linux seal/copy/audit commands) | ad6b7429b78361815e66bb8c15a3150812b405851a1e5332e531ab893bac92bd |
| original-primary/failure-context.json | dd6a9f61fa62866053d09c3b986f1c27880ee55fc0eeaeef6d07376dc334c2f9 |

Review the exact candidate/export and original driver recipe, authenticate the
complete packet/ELF/compiler closure and all70 actual runtime commands, then
publish the bounded component disposition. The full continuing finish line
requires its original integrated/native evidence and owner completion statement;
this finite pass does not close it. No new R15 reservation or abstraction exists.

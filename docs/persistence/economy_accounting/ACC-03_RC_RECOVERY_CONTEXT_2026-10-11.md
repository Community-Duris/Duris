# Native quest recovery context regression failed at link

ACC-03-RC ran the unchanged existing regression against frozen source
`6022c0ad7feab34c13e27738d4295b65b8d010ac` on 2026-10-11. The compiler driver
returned exit 1 at the final system link because its source list omits three
real providers. One compile attempt, zero successful linked builds and zero
runtime attempts were recorded. This is a genuine executable gate failure;
no component, sanitizer runtime, gameplay or release pass is claimed.

## Exact source and command

Owner chat `01a129b4-a57a-76e0-baec-42b3dd2fe62f`, job
`ACC-03-RC-6022c0ad7`, assigned branch `codex/accounting-accel-world`.
The worker checkout before this report was `2f33f32c253da91b774eefdc316a965505f29083`;
its runtime/test files equal the frozen launch source. An independent D: export
of all `src/` and the original runner/test contained 1,422 files, 35,071,468
bytes. All export hashes and the complete source file set remained unchanged.

The granted host command was:

```powershell
& 'D:/Dev/Tests/Duris/accounting-accel-world/ACC-03-RC/Launch-Acc03Rc.ps1' -GrantPath 'D:/Dev/Temp/accounting-acceleration-bootstrap-20261011/ACC-03-RC-resource-grant.json' -GrantSha256 'e87ecac7a1830fe272a70b19bac7d1d29df923975ec12a33e84c6becea36014e'
```

Its inner command, with cwd `/source`, was
`python3 -B -u tests/async/test_native_quest_recovery_context.py`.
The original runner compiled its 15 listed translation units with:

```text
g++ -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror
    -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie
    -pthread -Isrc -ffunction-sections -fdata-sections
    <original 15 source paths> -lcrypto -lz -Wl,--gc-sections -Wl,--wrap=_Znwm
    -o /source/bin/tests/native-quest-recovery-context-xzju89ml/native_quest_recovery_context
```

The complete actual argv and all 15 source hashes are in the original retained
`receipt.json`; no compiler/linker flags or predicates were changed. The original
20-second runtime deadline and ASan/UBSan settings were preserved, but no native
binary was linked or executed. Compiler-driver elapsed time was 76.958 seconds;
the outer helper ran 92.027 seconds and returned exit 1.

## First actual failure and narrow repair request

The linker reported these six distinct undefined symbols from retained
`src/item/item_transfer_command.c` functions:

| Missing symbols | Existing definition provider omitted by the runner |
| --- | --- |
| `native_quest_cost_projection_encode`, `native_quest_cost_projection_decode` | `src/economy/native_quest_cost.c` |
| `native_quest_coin_give_project`, `native_quest_coin_give_encode`, `native_quest_coin_give_decode` | `src/economy/native_quest_coin_give.c` |
| `lockpick_retirement_payload_valid` | `src/item/lockpick_retirement_continuation.c` |

All three definitions were checked in the frozen source. The separately reviewed
repair should add these real providers to `SOURCES` in
`tests/async/test_native_quest_recovery_context.py`, preserving its test body,
flags, allocator observer, sanitizer settings, deadlines and qualification label.
This repair was not implemented or rerun in this attempt. Additional link or
runtime findings remain possible; a source-list proposal does not qualify them.
Maintained runtime ownership still requires the primary's published acknowledgment.

## Resource identity and release

Actual container `duris-build-a`, CID
`dbc76721b8b99ad2f7ee2f9fb6dc635897bb77cb9d7bb4d894970ea5e9bafa49`,
used pinned image
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`.
GCC 13.3.0 and actual sanitizer-library hashes were observed. Direct D: source,
build, evidence and cache mounts were recorded; the runner's required
`ROOT/bin/tests` layout was preserved without a BIN_ROOT override. Private Linux
`/t` tmpfs verified mode 0600 and matching process/file UID. Actual cgroups
enforced one CPU, 1,536 MiB memory and zero swap; network was disabled, restart
policy was no and foreground auto-removal was enabled. The outer deadline was
900 seconds with five-second termination grace. No SQL service was used.

Container start was `2026-10-11T08:23:52.26175931Z`; the lease was RELEASED at
`2026-10-11T08:25:25.1692215Z`. The exact-owner/job/token inventory was empty,
the container was absent and the matching global lock was removed in finally.
Thirteen terminal evidence files were sealed and independently rehashed without
mismatch. Unrelated services, images, volumes and historical evidence were preserved.

## Retained evidence

Local evidence root: `D:/Dev/Tests/Duris/accounting-accel-world/ACC-03-RC/attempt-01`.
Original fixture artifacts:
`D:/Dev/Builds/Duris/accounting-accel-world/ACC-03-RC/bin/tests/native-quest-recovery-context-xzju89ml`.

| File | SHA256 |
| --- | --- |
| `artifact-manifest.json` | `32dbb848d0d4f5334bfb41f7cf7795df235087b2178218ceda7d81afd39f1d61` |
| `RESULTS.json` | `be2cdf436374e0c1ea17e8cb075d23ea7e8d9140a91ce2ca5b1581a4c73d425a` |
| `LEASE.json` | `4ca54d027167421728f899c686e733c051a973db003ecad66a8ec48e92bcffc1` |
| Original `compile.log` | `6200bf033272cde044f71c1dd27422e4fa3298abeb39201ab287572a7f656569` |
| Original `receipt.json` | `3ae483ae9535f7b9523fbe8ff2fbe32c7796a63fca12b650596a88a988e34003` |

The prepared receipt binds the helper/source/archive/runner/test hashes and the
separate resource grant. The gate remains failed and unqualified. Genuine native
birth/admission, SQL/flat acceptance, publication/ACK, cold gameplay recovery,
required architecture and release gates remain open.

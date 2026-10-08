# Ordinary pickup component handoff â€” 2026-10-08

## Delivery and import boundary

`tests/async/test_ordinary_pickup_publication_runtime.py` makes the reviewed private
ordinary-pickup characterization independently runnable. It preserves all eight
historical controls and does not execute the existing retention test's runner.
This delivery adds that one test and this handoff only. Production files and every
existing test remain unchanged.

The accepted [publication reservation](ORDINARY_PICKUP_PUBLICATION_RESERVATION_2026-10-08.md)
at owned baseline `0be75e3cb9057f72939d7def11a3727eec337233` remains the ownership
finding and conditional production reservation. Its former proposal to append a
section to the existing retention test is superseded by this isolated new test.
No production publication flag, room-source restore, player save/current proof or
native continuation implementation is included or newly authorized.

The executable target is published primary
`df0570c5456d4d747ca1320ce958c1db52bb08fd`, with source tree
`833d3085815b396861ad18a77635412212381e4b` and tests/async tree
`790f367adf805a69d53aac6460938f5c921f9136`. Private primary owner implementations
remain unavailable. The test does not import, approximate or qualify those owners.
It is an importable test patch for the published primary source, not a claim that
the older R0â€“R14 source in this owned worktree is a compatible execution base.

## Extraction, unchanged owners and doubles

The new test parses `tests/async/test_publication_retention_runtime.py` with
`ast.parse`/`ast.literal_eval` and requires exactly one string-valued `HARNESS`
assignment. It never imports that module or runs its top-level compiler/controls.
It takes the dependency prelude before `publication_callback`, removes only four
unused fixture variables and the original registry lookup/revision/apply doubles,
and links the actual runtime registry. Exact replacement markers refuse drift.

The existing `_paths.extract_function` extracts the actual `find_live_item_uid`
and `item_get_completion` from the candidate's source. Context/enums come from
bounded original declaration slices. Both function hashes are checked before
compilation; drift requires explicit review rather than silently changing the
historical characterization:

| Original callback/helper | SHA256 |
| --- | --- |
| `find_live_item_uid` | `214894bd143f18e2d302fa8544d531daab35aeef2807f158482471e89831e027` |
| `item_get_completion` | `dbf71bcb2118320aeee5dc7270cda6466467dc683a34a41e8fc5acdf59baa4ea` |

The generated C++ is byte-identical to the reviewed private component:
`d7cb458b1fa7f9598dcdfea5cd98326fc1186391a9e4e9310530701a12ad623c`.
Its real providers are the movement owner, runtime registry, transfer command,
character identity, coordinator and journal, with their actual codecs/accounting
shape validators and link dependencies: 21 unchanged C providers in total.
The appendix freezes all 27 directly selected source/fixture/helper inputs.
The entire archived source/test trees are authenticated separately in private
validation, including transitive headers.

The full [reviewed double inventory](ORDINARY_PICKUP_PUBLICATION_RESERVATION_2026-10-08.md#private-component-feasibility-and-bounded-evidence)
still applies without changes to generated C++:

- Accounting authority returns inactive. Its reused craft-only preparation double
  is not reached. Commands are schema 1 with `__NO_MYSQL__`.
- `apply_transfer` decodes the actual command and encodes a controlled result;
  there is no SQL transaction, active intent acceptance or accounting execution.
  `rejected` is only a terminal-outcome branch: error 0 and incremented result
  clocks are not a canonical SQL rejection receipt.
- Snapshot capture produces one synthetic UID/VNUM literal. `get_with_phase`
  counts calls and performs a small controlled carried-link effect, rejects, or
  throws before/after that effect. No native subtree/forest capture, actual native
  handler, selection/capacity, text, dirty-save, light or weight proof is supplied.
- Reused heap/string wrappers, output/alert/dirty bindings, cross-owner busy/save
  checks, craft/creation/collector helpers and fixture thread/global-list bindings
  have their exact previously disclosed behavior. Alert increments a counter;
  the relevant busy/save bindings return false. These do not prove concurrency or
  save exclusion. The selected ordinary case does not enter creation/enrollment.
- Corpse/NPC, panic and link-only drop, restored-save, held-retirement, lockpick
  publication and literal-capture dependencies abort if reached. No successful
  response is supplied for those unrelated owners.

## Historical controls and interpretation

Each case is a fresh process and journal. Submission uses the actual ordinary
reason/context/callback shape directly; it does not execute `do_get` or a login.
The real worker/coordinator produces completion. Before gameplay dispatch, the
assertions require zero journal records, no item/player coordinator fences and no
publication-pending operation. No completion is fabricated and no ACK is invoked
by the fixture.

| Case | Required historical observation |
| --- | --- |
| `success` | Actual registry advances room â†’ player; pending clears; doubled placement runs once and carries the object. |
| `rejected` | Terminal outcome leaves registry/source room and does not invoke placement; pending clears. This is not authenticated SQL rejection. |
| `missing_actor` | Pending and room registry remain after completion and an empty pulse; readiness then publishes once and clears pending. |
| `replacement_body` | The same waiting state resumes onto runtime 7002 for the same PID. Runtime 7001 remains registered but absent from the character list; this is controlled body selection, not authentic logout/reconnect. |
| `stale_topology` | Actual callback refuses mismatched source room, alerts once and never invokes placement; registry is already player and pending has cleared. |
| `handler_rejected` | Doubled placement returns rejected; actual callback alerts once; room placement remains while registry is player and pending has cleared. |
| `throw_before` | Exception from the placement double escapes actual callback/movement invocation after pending erasure; room placement remains and registry is player. |
| `throw_after` | The same exception escape occurs after the doubled carried-link effect; pending has cleared and registry is player. |

Subsequent empty pulses/readiness must not repeat the placement dependency. These
assertions describe current historical semantics. A later genuine retained owner
requires separately reviewed production changes and corresponding assertion
updates; passing this test does not make current behavior a desired retention
contract or justify weakening controls to preserve a PASS.

Missing actor and registry failure do have real protections. The unchanged
`tests/async/test_item_movement_input_queue.py` guards registry failure and busy
input behavior; that original control is preserved, not rerun or replaced here.
The original retention test's retained give, runtime identity, craft/progression
and uncertainty controls are also untouched and not executed by this new test.
Authoritative player load and custody/save guards can recover/protect durable
inventory. An erased live callback continuation is not evidence of irreversible
item loss. The reservation traces those source-backed protections in detail.

## Executed candidate and exact commands

Private task root is `D:\Dev\Temp\ordinary-pickup-component-20261008`.
`git archive` exported only published primary `src` and `tests/async` into
`candidate`, then the exact new test was copied into its test directory. Archive
SHA256 is `7402b5ebd87e7e6676e64b3dd597829461d280c0b5ed9d50956fc305c9c24d1f`.
Existing candidate files remain byte-identical to that Git tree; the new test is
the sole candidate addition. No maintained or private production overlay was used.

The actual launch from PowerShell was:

```powershell
wsl -d Ubuntu-22.04 -- env PYTHONDONTWRITEBYTECODE=1 TMPDIR=/mnt/d/Dev/Temp/ordinary-pickup-component-20261008/primary-evidence python3 /mnt/d/Dev/Temp/ordinary-pickup-component-20261008/candidate/tests/async/test_ordinary_pickup_publication_runtime.py --build-root /mnt/d/Dev/Builds/Duris/ordinary-pickup-component-20261008/bin --evidence-root /mnt/d/Dev/Temp/ordinary-pickup-component-20261008/primary-evidence --journal-root /dev/shm
```

The test records the complete compiler argv and all inputs in `results.json`.
It uses original C++20 flags with `-Og`, `-Wall -Wextra -Wpedantic -Werror`,
`-D__NO_MYSQL__`, pthreads, section GC, ASan/UBSan, no PIE, zlib and libcrypto.
Compile deadline is 900 seconds; each case deadline is 30 seconds. Runtime settings
are `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

All generated source/binary outputs go beneath the supplied D: build root.
Compiler temporary files and evidence go to D:. Journal directories must pass
owner-only POSIX permission checks before compilation. The selected `/dev/shm`
directories are RAM-backed; completed journals/logs are copied to D: without
weakening DrvFS permissions. This is journal/checkpoint mechanics, not disk,
power-loss, crash/restart or active/native qualification. Default roots use
`BIN_ROOT` or repository `bin`, the process temporary directory, and `/dev/shm`
when available; pass explicit roots as above on this Windows host. Each run
allocates new directories and retains evidence without overwriting earlier runs.

The first wrapper preflight incorrectly required a unique abbreviated
`get_with_phase` marker, but the source has both declaration and definition. It
failed before compiling or submitting a command. The final wrapper selects the
exact declaration; no generated C++ or runtime expectation changed. The failed
wrapper is preserved privately as `preflight-marker-failed.py`.

The final launch exited 0. WSL Ubuntu-22.04 g++ 11.4.0 compiled the component
in **441.994 seconds** with an empty diagnostic log. All eight cases exited 0
under the recorded sanitizer settings, with the historical observations above.
No runtime assertion or generated C++ changed after qualification. The two setup
checks (wrapper marker preflight and owned-base dependency refusal) are distinct
from these eight successful primary component executions.

Private evidence is
`D:\Dev\Temp\ordinary-pickup-component-20261008\primary-evidence\ordinary-pickup-w3nodefk`.
The generated source and executable are under
`D:\Dev\Builds\Duris\ordinary-pickup-component-20261008\bin\ordinary-pickup-98wd9jws`.
The actual fresh journal root was `/dev/shm/ordinary-pickup-h6z0fbki`.
Validation authenticated all **2,816** existing candidate files and all **27**
direct inputs; only the new test was added to the source/test candidate.

`D:\Dev\Temp\ordinary-pickup-component-20261008\evidence-index.json` authenticates
30 artifacts: results and compiler log, eight case logs and copied journals,
composition and compatibility records, validation and its scripts, the failed
preflight wrapper, final new test, generated source and executable. The index
does not contain or authorize a production/private owner implementation.

| Final artifact | Exact pin |
| --- | --- |
| New test Git blob | `c788c948d340ba0dba03c70776d74faa2d04063a` |
| New test SHA256 | `b3a2a62c28f194661e7b73c20acb9d83b01ac44196c83138c14d76b0ebb4be1b` |
| Qualified executable SHA256 | `6f5d0d52521cc749be72b8bc9020bf98b360fb5164008eb9dabf355b46b572b4` |
| Results JSON SHA256 | `a01d299be53c689bf0a19fae78b0e0431e9fce7256e9dcb4adaa016516165f6d` |
| Evidence index SHA256 | `5db5aeb4c7b021cd70f491ca99e3e06ad9febc0b9c3e674b8d74e1a197ece960` |

## Preserved R0â€“R14 base compatibility

On owned baseline `0be75e3cb9057f72939d7def11a3727eec337233`, the existing retention
fixture remains exact blob `6af4bb93605fa07c31b4ea42026100d91cbe3843`, identical to
published primary. Both extracted callback hashes also match. Among 21 selected
providers, ten are byte-identical, eight differ and three are absent:

- `src/economy/native_quest_cost.c`
- `src/economy/native_quest_coin_give.c`
- `src/item/lockpick_retirement_continuation.c`

The new test's owned-base launch exited 1 **before compilation**, reporting those
three missing published-primary providers. It is not runnable against this older
source set and is not a passing owned-base regression result. No provider copy,
conditional fallback, interface shim or production edit was made to hide that
dependency. Detailed per-provider hashes and the exact owned command/output are
in private `owned-compatibility.json`/`.log`.
The actual owned-base command, launched by the private compatibility script in
Ubuntu-22.04 with bytecode writes disabled, was:

```bash
/usr/bin/python3 '/mnt/c/Users/alexa/.codex/worktrees/ac24/NewDuris Max/tests/async/test_ordinary_pickup_publication_runtime.py' --build-root /mnt/d/Dev/Builds/Duris/ordinary-pickup-component-20261008/owned-bin --evidence-root /mnt/d/Dev/Temp/ordinary-pickup-component-20261008/owned-evidence --journal-root /dev/shm
```

Import the two-file patch into the reviewed published-primary source lineage,
with its exact source/fixture/header contracts, and rerun the test there. Callback
identity alone does not establish compatibility of coordinator, registry, command
or accounting owners across these bases. If a future primary changes one of those
owners, compare its full interfaces/control flow and review the historical
assertions before reuse. Unavailable private owner changes are not qualified.
This delivery does not reserve their files or broaden the production reservation.

The finite component delivery leaves the broader native Goal BLOCKED and
unfinished. Existing drop, quest/SHOP, temporal QP03, Plan 5 and all completed
preparation bundles remain preserved. No full native build, old acceptance-suite
rerun, server, DB, migration, activation or operational action was performed.

## Frozen direct inputs

The new test's own final code pin and execution artifact pins are recorded above.
The following inputs are exact published-primary preimages; transitive headers
also belong to the authenticated source tree.

<!-- INPUT_PINS -->

| Published primary input | Exact Git blob | SHA256 |
| --- | --- | --- |
| `src/account/character_identity.c` | `f7c46058bc7881d68404648dfbdca09d1f25edaa` | `390e2103d528308457e811925137adbd2647ee73844d4a610994ff4be1d52388` |
| `src/classes/necromancy.h` | `24a022cf2e586a8ca320c2deea5d5fd9ec6a2b8e` | `76c6faeadd1a42b33a4ca2682b655482af4417569d37783418386b42eea6ca65` |
| `src/cmd/actobj.c` | `a2114fddb6816f1534488ff11457a1001d47a14c` | `bfb0043ec8abb0e5526cdb76e458f97d6e5612743b65c7640072bb13d16d62f2` |
| `src/combat/chaos_pouch_ledger.c` | `a0c8363bc13ba6926ef320ded9dda2bba67a4984` | `25ecb4cfe9838ff2ef3c021c5df48dc18936fea3776c135065c3f71049a9c978` |
| `src/combat/chaos_pouch_publication.c` | `bd2f920acc95209c04cfa7e5cf15e6932b69853e` | `b224b58e64a495a0f90a428f04070bee632e729f1488f0a31b7f557a6034c9d8` |
| `src/economy/economic_accounting_intent.c` | `00db5654239f221392bc79b5bdc5ee6046d55a43` | `c61810173a255042e047d7e66b2d690d62d40b7ffe3910473998a4f9bb9cc278` |
| `src/economy/economic_accounting_plan.c` | `8204617fcf1ea5f322f06897f9c5c8848f9aa38b` | `53a3812a24cf4a3c9cfe36c368c6026f897ff6f38bc5c7aec14c0cbcaa75c4e8` |
| `src/economy/economic_accounting_types.c` | `f1ada31fc487e5649bda982d30482ace1e6ce009` | `5c2117874526ba9a1f93b3313eb9c13782e50c9d0f1d55763449089db68b36f1` |
| `src/economy/economic_source_event.c` | `f91b9c3eb94dfd2900dfa39e97391cad7966f301` | `acefc0d801998c5bcc28c0841d774e423bb930bfc0992d3b443fec592ff0fd08` |
| `src/economy/item_transfer_accounting.c` | `b7012f63c3ce96573eaf23c0424d6447f71e74ac` | `33c6e90ff34c91a8222609f69e3b113116e0090a1538294b747559797d9b3f85` |
| `src/economy/native_quest_coin_give.c` | `eaf9fc466bfda32e6acd33b88bb6bed9fa81f0a2` | `88543581facf2dca16e646a7be068539106e956142f6ec3eb473e32b5bab26f3` |
| `src/economy/native_quest_cost.c` | `439ceade2196d33ac60e8428a12fda2f19564837` | `036978c11d968b5dcda06e7bbc06ee7715441b4bc9c433a4b26879f16a8e2ca5` |
| `src/economy/shop_trade_recovery_manifest.c` | `1dc5d70e0c8dd13ade685a80a183a7783e827abf` | `1a1543b2f8e26aa665eea052e0e6184dcd7e02a2b3da1680450737fc231139c3` |
| `src/item/craft_pouch_mutation.c` | `0de5f65698c85a056303ab35b1156a4feeb0b650` | `eae5c78864187b0e2ba4e0d596b1d2476d972ccf227994816e31b103254a2b30` |
| `src/item/item_movement_transaction.c` | `9d2ce19fd843eef7ce2bda7c41348471b5a2864b` | `b41d72ba71a5a80e10d8538d4f272bc87013bfc00ae49f344459ff1ec7ee02a5` |
| `src/item/item_ownership_runtime.c` | `cf0aae0dfa3ce2d7397d357127ec1b6e3c8074ba` | `1e9e5ae868e78499349efb93beba895708a53a586e553ff334bff0e06686152b` |
| `src/item/item_transfer_command.c` | `637051605ad1b1b27b9254eb510acb347852f5e9` | `2b5c12b0c4a9437643b2e2285333b8265d871155c25f306400c5f07e568550f9` |
| `src/item/lockpick_retirement_continuation.c` | `c95ca2ccf4af9d30265036ecebc709d9be81865f` | `fcf706b98339b2a5266e3ea1897c93d399820c621a85c0aef99dcaf14007e0a3` |
| `src/persistence/critical_command.c` | `f7bc9b86f0fbf3b75fe6ae2b47ed62442f1d4763` | `e4e998c275831a7594b0abd1cd54e0aeba46b14b57f97d62a68ad479fa5d1f91` |
| `src/persistence/critical_command_coordinator.c` | `43a5b4f15c9171703e77b89b84bbc0c6f3a49b16` | `1b1313b299e4cb16c1be26d0d3b037e3d229ccbf6183bf84ea4461492f0ba936` |
| `src/persistence/critical_command_journal.c` | `a64124327c421ff928597619600597b534fbc600` | `5dae1b7fe3f02dbc262648403e40eb31431cbfd79767c8cec57e2c9bc94fc7c2` |
| `src/player/player_snapshot_codec.c` | `27d440a90d714fd1cba95720edd9e59eec976844` | `80cadd1a287c76916bd2afd7ed30e4e588ee4a1b9c63a9d085ee2496d665f03c` |
| `src/world/quest_mobile_native_reference.c` | `f0308d6b79912af6e1b9233307b66ee66cc76497` | `d5273cdee11447dc307c0e14114785d332ccd130bfb59982a811e0f1d68b6d01` |
| `tests/async/_paths.py` | `7675d09df7aaeb1db606b598b821c3d4c4f4c8f2` | `bb1ffca804d125835894427e22ef3b286e3555e8ab9af75978a817bef00958e6` |
| `tests/async/character_identity_test_fixture.h` | `b72342cb7b917778d6d69f91162ca63c1f6854ad` | `2871e9fee9dadf7b8af4e29eb7880897beac0a7aecf738bc9c3ff6faf1c7ef04` |
| `tests/async/contract_text.py` | `a8b7a1279f112971423b04c8861b462e79605dff` | `65202fb525855c647053c98eded3204d332b9e747faa29deeb1ad1759439d9a5` |
| `tests/async/test_publication_retention_runtime.py` | `6af4bb93605fa07c31b4ea42026100d91cbe3843` | `f667e11708f29a34652e937e917c5f19496aa27f0038b4f9d46167efdaaa5f94` |

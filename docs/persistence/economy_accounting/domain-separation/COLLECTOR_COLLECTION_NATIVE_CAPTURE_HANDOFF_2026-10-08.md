# Collector collection native capture acceptance — 2026-10-08

This delivery adds a bounded actual-provider acceptance runner and native fixture
for collection preparation. It executes the published Collector adapter, ordinary
player snapshot capture and codec, custody runtime, identity helpers, Collector
command validation and critical command envelope codec. The qualification ends
at owned capture values and command bytes. No production body changes.

The continuing Plans 1–5/applicable original R1–R8 Goal remains **BLOCKED and
unfinished**. This delivery does not change its lifecycle or any automation,
coverage registry, Plan 5, primary work or independent quest-fee stream.

## Frozen candidate and files

The executed source is published primary
`20510d07da21759396bbe8775150f1d9aafc4233`, exported with `git archive` for
`src` and `tests/async`. The owned checkout contains additional work; it was not
used as a substitute for this candidate.

| Input | Pin |
| --- | --- |
| Primary `src` tree | `833d3085815b396861ad18a77635412212381e4b` |
| Primary `tests/async` tree | `790f367adf805a69d53aac6460938f5c921f9136` |
| Archive SHA256 | `e696c79a8061a6e13a2cfb883f8c158a3913365d0cb1327d60cf01ffaa06afc5` |
| Archive size / original files | 43,520,000 bytes / 2,816 |
| Parent delivery | `b49a8612bc189436a4a4d0a6f3be997c4d6c2b4b` |

The original 2,816 file hashes are retained in the private export manifest and
checked again after qualification. The only additions to the candidate export
are byte-identical copies of the two new test files. This public primary has no
`src/economy/collector_collection_image.h`; the inline ordinary image calculation
in `collector_collection_preparation.c` is the body exercised here. Closed R0 is
not reopened, copied or presumed adopted.

The three-file delivery relative to the parent is:

- `tests/async/test_collector_collection_native_capture.py`: bounded build,
  symbol, dependency, runtime and artifact accounting.
- `tests/async/collector_collection_native_capture_harness.cpp`: native fixtures,
  independent expected values and 82 separately invoked controls.
- This handoff: exact qualification, seams, results and remaining boundaries.

| Final test file | Git blob | SHA256 |
| --- | --- | --- |
| Python runner | `3eeb3a849df0dbe9916314b84e5df86150939a64` | `fad1dac27af2261b2de8719b83a061ece901f1e09c9b2083b474f84efe20bc81` |
| C++ harness | `c8fe0282f82af7b912193a90d93516709829fa13` | `6f06b98df8bae686d5bb4050e0b9ec325c78d3930af77b7dae758c9347b28cb0` |

See the preceding
[Collector authority map](COLLECTOR_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md)
for source/design ownership. Its source trees are identical to this published
candidate's trees. This acceptance executes its one bounded follow-up reservation.

## Genuine providers and explicit seams

The runner compiles these twelve **complete production translation units** from
the frozen candidate, plus the test driver:

| Provider | Exercised responsibility |
| --- | --- |
| `src/economy/collector_collection_preparation.c` | Prepare, eligibility, current native comparison, full original-root authority rows and singleton blob. |
| `src/economy/collector_policy.c` | Genuine enrolled record and due policy. |
| `src/economy/collector_codec.c` | Production Collector codec dependencies. |
| `src/economy/collector_command.c` | Collector payload topology/identity validation, command construction and payload codec. |
| `src/player/player_snapshot_capture.c` | Ordinary native tree snapshot, masks, properties, affects and extensions. |
| `src/player/player_snapshot_codec.c` | Real singleton encoding/decoding and its limits. |
| `src/item/item_ownership_runtime.c` | Real registry hydration, lookup, owner revisions, peek, forget and reset. |
| `src/item/item_transfer_command.c` | Real owner identities, equality, keys and corpse/Collector identity helpers. |
| `src/economy/currency_command.c` | Production command dependency. |
| `src/persistence/critical_command.c` | Actual command envelope, revision fences, encode/decode/equality. |
| `src/world/handler.c` | Actual `isname`, used by unique-item eligibility. |
| `src/core/utility.c` | Production dependencies of the native/command providers. |

| Production body | Git blob at the frozen primary |
| --- | --- |
| `collector_collection_preparation.c` | `90bca1b00e8991650a7cc3958866fcefcf732515` |
| `collector_policy.c` | `931ed49946224e9eefed11071930383049d46dcf` |
| `collector_codec.c` | `b129bad185d1a6aefb9f1bde93ebeb6974f79cc5` |
| `collector_command.c` | `ac2f432e46e29335a4b1e0fdc5340fa831105973` |
| `player_snapshot_capture.c` | `9aacf75b74b45f1d2f0911e7325b9d7a3125921b` |
| `player_snapshot_codec.c` | `27d440a90d714fd1cba95720edd9e59eec976844` |
| `item_ownership_runtime.c` | `cf0aae0dfa3ce2d7397d357127ec1b6e3c8074ba` |
| `item_transfer_command.c` | `637051605ad1b1b27b9254eb510acb347852f5e9` |
| `currency_command.c` | `29ba7ccbadf7d8a19a3a009b02c514fea9a11a14` |
| `critical_command.c` | `f7bc9b86f0fbf3b75fe6ae2b47ed62442f1d4763` |
| `handler.c` | `0054c7db2f7cb990dfde047e6de09250d54670e3` |
| `utility.c` | `92bdffd57f8979d9531251d1b049887505d7715d` |

Git blob and SHA256 pins for each body are also in private `qualification.json`.
`-MD` dependencies capture the actual repository and system-header closure;
compiler executable, cc1plus, collect2, assembler, linker and resolved shared
libraries are separately hashed. `defined-symbols.txt` confirms linked tested
providers. Whole-TU compilation does not imply whole-TU runtime coverage.

The test supplies only synthetic native `obj_data`, object-list pointers, two
rooms, six prototype/index entries and the corresponding globals. Stable object
addresses use a `std::deque`. It constructs an in-memory enrolled record through
the actual policy API and preloads the actual custody runtime with manual rows.
It supplies `nevent_require_game_thread`/`nevent_is_game_thread` using the fixture
thread and aborting `panic_corruption`, `panic_corruption_int` and
`fatal_boot_error` diagnostics. These are fixture/thread/fail-stop scaffolding;
there is no server thread or scheduler qualification. No diagnostic accepts a
capture or substitutes a durable result.

There is no copied or replaced capture, codec, custody registry/revision,
identity, `isname` or eligibility function, nor an accepting SQL, save,
materializer, coordinator, journal or publication double. Function/data sections
and linker garbage collection exclude uncalled detach and unrelated server
functions. The runner explicitly checks that
`collector_collection_detach_live` is absent from the ELF. Detach is unexecuted.

Command construction produces an envelope with acceptance time zero. The fixture
sets a disclosed synthetic `accepted_at_usec = 123456` solely to round-trip the
real critical envelope codec. This is structural codec validation, not admission,
an ACK or a coordinator receipt. The generic item-transfer command encoder is
not exercised; its genuine identity/key helpers are exercised directly and by
the Collector command builder.

## Independent oracle and checks

The principal fixtures cover room and PC-corpse locations with both selected root
and selected nested container. Descendants, a sibling outside the selected subtree
and an ancestor make the complete original root observable. UIDs deliberately
disagree with traversal order: 50, 90, 200, optional 500, 700. Expected authority
rows are written manually in UID order with independent parent/root, VNUM, state
and revisions. Corpse identity is manually `(42 << 32) | 17`; room identity is
VNUM 9000. Source owner revision is 9, destination Collector 77 revision is 0.

The expected ordinary singleton is written field by field, without reading a
native object or a captured snapshot. It includes UID 200, generated key 17,
VNUM 5001, normalized parent -1/equipment 0, all four masked strings, type/wear/
anti/extra flags, five bitvectors, eight values, six timers, material, cost,
condition, craftsmanship, four fixed affects, a dynamic affect and an extra
description. Own weight is the manual value 7, after subtracting direct child
aggregate weights from the native aggregate. The **real codec** encodes this
manual expected singleton and those bytes must exactly equal the prepared blob.
Real decode/re-encode, Collector payload round-trip and critical envelope equality
add structural checks. A second prepare checks determinism; it is not the oracle.

Every prepare audits the same native graph before/after. With preseeded owner
keys, prepare, refused prepare and live comparison also audit actual registry
rows and observed owner-key projections before/after using noninserting peeks.
Raw representations of the same trivially copyable native instances are only
write-detection evidence; expected capture values never come from those bytes.
Refused prepare must preserve both the original output pointer and its complete
payload representation. Failed live comparison preserves the caller's selected
pointer; failed Collector payload encoding preserves a sentinel byte vector.
No strong-output claim is made for failed decode.

`new_owner` separately starts without a destination key: the genuine owner query
creates revision zero, keeps item count unchanged, produces the manual expected
payload and then remains stable across repeated prepare/comparison. Its first
prepare deliberately permits registry change; it does not claim a complete
item-row before/after audit for that first initialization. Other prepare controls
preseed the key so that this behavior cannot conceal an unexpected write.

| Controls | Acceptance or refusal established |
| --- | --- |
| `room_nested`, `room_root`, `corpse_nested`, `corpse_root`, `new_owner`, `newer_item` | Manual full-root/singleton expectations, identities, current revisions and deterministic genuine command bytes; current item revision may exceed enrollment. |
| `ordinary_mask`, `live_eligibility_omitted` | Ordinary capture omits unmasked strings; a changed masked key refuses comparison. Live comparison does not rerun eligibility, while fresh preparation still excludes a newly unique item. |
| `root_limit`, `root_limit_plus_one`, `blob_budget`, `string_budget`, `depth_budget` | Actual 3,000-row limit and 3,001 refusal; adapter refuses an otherwise encodable singleton above 128 KiB; capture string/depth bounds refuse. The large-root positive control checks count, live match and command acceptance rather than 3,000 manual rows. |
| `negative_child`, `negative_selected`, `weight_sum`, `zero_weight`, `max_weight` | Negative aggregate inputs and child sum exceeding selected aggregate refuse; legal own weights zero and INT_MAX encode. |
| `native_duplicate_nonselected`, `command_duplicate`, `command_parent`, `command_cycle`, `command_owner_max` | Native pointer traversal can accept identical nonselected duplicate UID rows; genuine command validation refuses duplicates, missing parent, parent cycle and maximal owner revision, with output unchanged. |
| `pre_take`, `pre_money`, `pre_corpse`, `pre_artifact`, `pre_transient`, `pre_norent`, `pre_nosell`, `pre_bound`, `pre_unique`, `pre_powerunique`, `pre_rnum`, `pre_duplicate_selected` | Actual eligibility exclusions and powerunique exception, invalid prototype and duplicate selected-UID handling. |
| `pre_claimed_player`, `pre_claimed_shop`, `pre_claimed_pet`, `pre_destroyed`, `pre_quarantined`, `pre_revision_max`, `pre_revision_old`, `pre_missing_registry`, `not_due` | Claimed custody, destroyed/quarantined state, missing/stale/exhausted revision and due-policy refusals. Pet identity uses genuine nonzero ID and context. |
| `sibling_revision`, `ancestor_revision`, `descendant_revision`, `row_owner_revision`, `row_owner`, `row_root`, `row_parent`, `row_state`, `row_vnum`, `row_revision_zero`, `owner_clock_only` | Full-root row changes refuse; a standalone owner-map clock advance with unchanged item rows remains a match, documenting the actual observation boundary. |
| `extra_child`, `missing_child`, `reciprocal`, `ancestor_reciprocal`, `sibling_cycle`, `global_cycle`, `duplicate_selected`, `room_vnum`, `room_reciprocal`, `corpse_pid`, `corpse_saveid`, `corpse_flag`, `corpse_reciprocal` | Changed membership/count, pointer reciprocity, cycles, duplicate selected identity and changed location identities refuse. |
| `literal_cost`, `literal_condition`, `literal_fixed_affect`, `literal_dynamic_affect`, `literal_extension`, `literal_string`, `literal_weight`, `literal_generated_key`, `literal_value`, `literal_timer`, `literal_flags`, `literal_mask`, `affect_cycle`, `description_cycle` | Ordinary selected singleton byte changes and malformed affect/description lists refuse current comparison. |

The test does not invent a global ownership census. Its audits cover fixture
nodes and observed owner keys. It does not infer global nonselected UID uniqueness
from pointer traversal, or owner-map revalidation from per-item rows. Native
weights are 32-bit and the root is bounded at 3,000 items; a 64-bit child-sum
overflow cannot be reached with this legal native shape, so no artificial native
64-bit-overflow proof is claimed. Allocation-failure injection is unexecuted.

## Reproduction, deadlines and results

Run in Linux/WSL with C++20, OpenSSL and sanitizer libraries. These exact runs used
`Ubuntu-22.04`, `g++-12` version `12.3.0-1ubuntu1~22.04.3`, the frozen export at
`/mnt/d/Dev/Temp/collector-native-capture-20261008/candidate`, and task-owned output
under `/mnt/d/Dev/Builds/Duris/collector-native-capture-20261008/bin`.

```sh
export CXX=g++-12
export TMPDIR=/mnt/d/Dev/Temp/collector-native-capture-20261008/compiler-temp
export BIN_ROOT=/mnt/d/Dev/Builds/Duris/collector-native-capture-20261008/bin
python3 "$TMPDIR/../candidate/tests/async/test_collector_collection_native_capture.py" \
  --source-root "$TMPDIR/../candidate" \
  --build-dir "$BIN_ROOT/acceptance-O1" --optimization O1
python3 "$TMPDIR/../candidate/tests/async/test_collector_collection_native_capture.py" \
  --source-root "$TMPDIR/../candidate" \
  --build-dir "$BIN_ROOT/acceptance-Og" --optimization Og
```

Output directories must be new. The retained private `qualify.py` invokes each
command with an additional 3,600-second outer deadline and logs the full command,
environment overrides, return code and elapsed time. Each compiler/version/tool
probe is bounded at 20 seconds; each independent TU compilation at 900 seconds;
link at 120 seconds; `nm`/`ldd` at 20 seconds; each runtime case at 90 seconds.
There are three independent compilation workers and no concurrent runtime cases.
Nonzero diagnostics and timeout partial output are retained. Private controls
exercise the exact runner's log helper: exit 7 retains stdout and stderr; a
0.2-second timeout returns 124 and retains partial output plus its timeout marker.

Both profiles use `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g`, ASan+UBSan,
frame pointers, non-PIE code/linking, function/data sections and `__NO_MYSQL__`.
Link dependencies are `-lcrypto -pthread`. Runtime uses
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

| Fresh final profile | Result | ELF SHA256 | Wrapper elapsed |
| --- | --- | --- | --- |
| `-O1` | **82/82 PASS**, exit 0 | `216b92c553e521f6059b438dc2f3328e774bc04a48563776fc8691c98fdf2f2d` | 120.798 s |
| `-Og` | **82/82 PASS**, exit 0 | `5a7e2c4377f0d838e19e6d1249c39a24a007a756a766fd60dce057e072ad36ee` | 93.155 s |

All thirteen compilation commands and each link returned zero in each final
profile. All 164 final case invocations returned zero and their expected PASS
marker. Compiler and link logs are empty; runtime logs contain only the expected
PASS marker, with no ASan/UBSan diagnostic. Both profiles retain their full
538-input dependency closure and resolved library pins, rechecked after execution.
Compiler executable SHA256 is
`88315fd2d961a1f4e4070b104d7bd4c7670df19d9949409879ba269466c196d3`.

The two initial private compile failures are retained: probe 1 found an incorrect
fixture declaration of `nevent_require_game_thread` (void rather than bool), and
probe 2 found a signedness warning in the fixture depth loop under `-Werror`.
Both fixture defects were corrected. Probe 3 subsequently passed all 82 controls;
its earlier binary predates the final envelope/sentinel checks and formatting,
so it is historical evidence only. Final profile results above use fresh complete
builds of the final files. No source provider was changed to make them pass.
The first post-run metadata check could not use Linux Git with the Windows
managed-worktree gitdir; WSL also could not execute Windows Git through interop.
Both failures are retained in `metadata-1-failure.txt` and
`metadata-2-failure.txt`. Git pins were then collected with Windows PowerShell/Git
and consumed by the successful Linux evidence check. These metadata corrections
changed no test, source input or acceptance binary.

`scripts/format.sh --check --file tests/async/collector_collection_native_capture_harness.cpp`
passes with clang-format 18.1.8. The final delivery also receives Python syntax,
unchanged export/dependency, artifact hash and three-file diff checks. A full
server `make`, broad regression batch, database, migration, server, journal and
native gameplay run are outside this bounded assignment and were not run.

## Evidence and remaining owners

Private evidence is retained on D::

- `D:\Dev\Temp\collector-native-capture-20261008`: exact archive/export,
  original input pins, scripts, wrapper commands/results/logs, deadline controls,
  `qualification.json` and `evidence-index.json`.
- `D:\Dev\Builds\Duris\collector-native-capture-20261008\bin`: failed probe
  artifacts, historical probe 3 and separate `acceptance-O1` / `acceptance-Og`
  objects, dependencies, compiler/tool/library pins, link maps, symbol dumps,
  per-case logs, `runs.json`, runtime profiles, ELFs and artifact manifests.

The evidence index hashes retained files other than itself, the candidate export
(already authenticated by its original-file manifest plus two test pins) and
ephemeral compiler scratch. Evidence is private and not committed.

Qualification is **capture/codec/custody/command acceptance only**. Detach,
materialization, durable compound mutation, SQL receipt/cache effects, purchase,
publication, player save exclusion, ACK, replay and cold restart remain
**UNEXECUTED by this delivery**. Public flat publication still refuses; this test
does not establish another publisher. The reported private `be8942cd` candidate
remains source-review/unexecuted evidence, not imported execution evidence.

The unavailable exact439 shutdown/SHOP/Collector candidate and original native
player/coordinator/publisher driver block their dependent native journey only.
They did not block this independent public-source acceptance. The next native
journey requires those exact owner inputs and its own bounded authorization and
qualification. This three-file branch does not start that journey or close the
continuing Goal.

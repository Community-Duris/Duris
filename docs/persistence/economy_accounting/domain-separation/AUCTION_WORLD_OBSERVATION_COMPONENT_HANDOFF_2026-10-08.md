# Auction world-observation component handoff — 2026-10-08

## Delivery and import boundary

`tests/async/test_auction_native_world_observation.py` adds executable acceptance
for the existing `auction_native_world_observe` boundary. The patch adds only
that test and this handoff. No production file, existing test, public API, schema,
shared authority, quest path, or Plan 5 implementation changes.

The execution target is maintained primary
`cda8aa6f65c72121e92d7c07efae7e91164d1050`, with exact source tree
`833d3085815b396861ad18a77635412212381e4b` and original `tests/async` tree
`790f367adf805a69d53aac6460938f5c921f9136`. The private candidate contains a
byte-authenticated export of those two trees plus the new test. The archive is
43,520,000 bytes, SHA256
`2d10a6848ba4bb61f4b30d6b7cb9daad6121a47b0a974c2c802d76f837939fff`.
All 2,816 original files were hashed before qualification and verified afterward.
The older production source in the owned worktree is not the execution target;
this is an additive test patch to import into the maintained primary candidate.

The [reviewed auction authority map](AUCTION_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md)
remains the domain boundary reference. This slice implements its first observer
acceptance component. It supplies no new native publication owner or authority.

## Genuine implementation and disclosed fixture support

The runner compiles eleven complete production translation units as C++20. It
does not extract functions, declaration slices, or another test's harness. The
anonymous census and physical-link validation execute inside the actual auction
translation unit. Ordinary and literal captures, forest codec/extraction, runtime
ownership lookup/hydration/reset, and character identity allocation/registration/
lookup/retirement use their genuine providers.

| Whole production translation unit / reused support | SHA256 |
| --- | --- |
| `src/economy/auction_native_publication.c` | `c4de84e0a4e9b52307e2320b36d86c74748bd58f9cd0234dd0d9e7941dd909ed` |
| `src/player/player_snapshot_capture.c` | `70e8dc17384d4969ce9741e09f6cb8b17d39da354afbd7662e6deb9404d6cd66` |
| `src/player/player_snapshot_codec.c` | `80cadd1a287c76916bd2afd7ed30e4e588ee4a1b9c63a9d085ee2496d665f03c` |
| `src/item/item_ownership_runtime.c` | `1e9e5ae868e78499349efb93beba895708a53a586e553ff334bff0e06686152b` |
| `src/item/item_transfer_command.c` | `2b5c12b0c4a9437643b2e2285333b8265d871155c25f306400c5f07e568550f9` |
| `src/account/character_identity.c` | `390e2103d528308457e811925137adbd2647ee73844d4a610994ff4be1d52388` |
| `src/player/inert_item_stage.c` | `931b52e9400b0a4b380f93f645cf8ba9f1cab17ec55ae11ff2d2738a9e1ba2cd` |
| `src/core/mm.c` | `8b9ab8933a82983656817106ce5e8b210b24df20c0b3c481224ab69995dc43e6` |
| `src/core/memory.c` | `77bc2042f3b33a0e134ac9815fa822fd1b9e8a4cd5fb6b78e7e34d1b491e5ab9` |
| `src/core/utility.c` | `1930fb6bbcea509818c5a9986d47086ef1033adf124e8d396cd73df7a7b60668` |
| `src/mob/studioproclib.c` | `299f91e89032d32ae01c492550010bf28d7f84077f4f62e6a5cf417f75bf1d4b` |
| `tests/async/character_identity_test_fixture.h` | `2871e9fee9dadf7b8af4e29eb7880897beac0a7aecf738bc9c3ff6faf1c7ef04` |

Function/data sections and linker garbage collection discard unrelated code.
The native pending-map destructor still requires the genuine inert-item,
allocator and procedure-chain destructors. `core/utility.c` provides the real
`IS_MORPH` and diagnostic implementation. No success-returning substitute was
added to satisfy link errors. Linking a provider does not prove every function
in it was exercised; native stage preparation/materialization and publication
remain outside this component.

The complete support inventory is:

- Synthetic `object_list`, `character_list`, `descriptor_list`, two rooms and
  their `world` pointer/bound, three object index entries and their pointer/bound.
  Objects, actors, PC data, descriptors, extra descriptions and dynamic affects
  are explicit fixture inputs. Their reciprocal links are constructed directly.
- The existing `character_identity_test_fixture.h` supplies registration checks,
  retirement checks and `nevent_require_game_thread`. `nevent_is_game_thread`
  compares the current thread with the fixture's main thread. The off-thread
  case uses a real `std::thread` and requires refusal. This does not qualify the
  server event loop or general concurrency exclusion.
- `panic_corruption`, `panic_corruption_int` and `fatal_boot_error` abort if
  reached. They never return success or suppress a tested rejection. No capture,
  UID census, codec, ownership, identity, placement, SQL, ACK or save double is
  supplied.
- The NORENT case hydrates the actual runtime registry with explicitly synthetic
  active player rows. Those rows are component inputs and are not SQL receipts,
  custody admission, or evidence of native durable ownership.

## Independent expected forests and acceptance controls

The oracle is a separately written values constructor plus explicit ordered
forest rows. It never reads a live object, captures a baseline as its expectation,
or calls `auction_native_expected_player_forest` to decide what the observer
should accept. Rows specify UID, VNUM, parent index, equipment slot, generated
key, type, all string masks/values, flags, weight/material/cost/condition/
craftsmanship, five bitvectors, eight values, six timers, four fixed affects,
dynamic affects and extra descriptions. Full player and selected forests are
compared through the genuine canonical codec. This checks capture/observation
against manual values; it is not an independent qualification of codec bytes.

Each case runs in a fresh process; observer cases first establish a valid
real-observer baseline. Every observer refusal uses prepopulated actor, player-forest,
selected-pointer and selected-tree sentinels and requires all outputs unchanged.
The values-only helper refusal checks separately preserve their output sentinel.

| Controls | Required observation |
| --- | --- |
| `carried`, `detached`, `absent` | Exact selected pointers/trees, full unrelated equipped/inventory rows and order. Detached models post-list removal: selected root/subtree leaves the player's manual forest while the selected literal tree remains exact. Absent returns null and an empty selected tree. |
| `claim_world` | Real observer accepts a manually constructed post-claim world: matching-VNUM insertion precedes the existing root; a later unmatched root enters at inventory head after equipment. Descendant parent indexes, root-only generated-key effects and unrelated rows are exact. An order perturbation refuses. This does not execute or qualify the placement handler. |
| `projection` | Supplemental genuine helper checks for list removal, multi-root claim insertion, parent-index rebasing, root-only generated key, level 56/57/58 and PID threshold, rejected/non-item actions, root/VNUM order, literal masks, duplicate UID and unchanged failure outputs. These helpers are not the observer oracle. |
| `descriptor_original`, `multiple_discovery`, `runtime_zero_absent` | Descriptor-original discovery works; repeated discovery of the same body pointer is valid; an absent PID with runtime zero and empty expected player forest is valid. |
| `replacement_body`, `duplicate_actor`, `runtime_wrong_pid`, `runtime_zero_present` | Replacement runtime, two bodies with the PID, runtime/PID mismatch, and a present PID with runtime zero refuse. The replacement body owns an otherwise identical complete forest and passes observation with its own runtime first, so forest mismatch cannot hide a runtime-selection regression. |
| `duplicate_selected`, `duplicate_world_uid`, `duplicate_child_uid`, `norent_duplicate` | Duplicate selection or physical UID occurrence refuses, including a NORENT duplicate invisible to ordinary save policy. |
| `dual_room_player`, `missing_reciprocal`, `wrong_carrier`, `wrong_child_reciprocal` | Multiple physical owners and inconsistent reciprocal links refuse. |
| `extra_child`, `missing_child`, `reparented_child` | Extra, missing or reparented descendants refuse against the complete manual forest. |
| `sibling_cycle`, `container_cycle`, `global_prev`, `global_next_cycle`, `character_cycle`, `room_people_cycle`, `descriptor_cycle` | Cyclic lists/containment and broken global previous links refuse within the bound. |
| `literal_changed`, `dynamic_changed`, `description_changed`, `equipment_changed`, `unrelated_changed` | Exact literal metadata, dynamic affect, extra description, equipment and unrelated inventory changes refuse. |
| `invalid_expected_parent`, `invalid_expected_mask`, `carried_literal_mask`, `invalid_location`, `zero_selected_uid`, `max_selected_uid`, `absent_present`, `detached_linked`, `detached_next_content` | Invalid values/topology or incompatible selected-location assertions refuse. |
| `bounds`, `depth`, `byte_budget` | Nine selected roots / ten refused; 4,096 player rows / 4,097 refused; 4,096 string bytes / 4,097 refused; selected depth 32 / 33 refused. Live string/object overflow is tested with valid expected rows before separately testing oversized expected inputs. A shallow 300-node literal tree with individually valid strings exceeds the 4 MiB aggregate budget and refuses. Pure depth validation is supplemental to the real capture/observer depth control. |
| `norent` | Literal selection retains NORENT root/child with full strings. Genuine ordinary capture omits them without active runtime custody and includes them with synthetic active runtime rows while retaining ordinary string-mask policy. |
| `off_thread` | Actual public observer refuses from a worker and preserves every output sentinel. |

The census one-million-node walk limit is not allocated/exhausted here. Cyclic
walk termination and snapshot/root/string/depth/byte bounds are executed. No
claim of exhaustive allocation-failure or every possible malformed-world proof
is made.

## Executed validation and retained evidence

Ubuntu-22.04 WSL used `g++-12 (Ubuntu 12.3.0-1ubuntu1~22.04.3) 12.3.0`, strict
`-std=c++20 -Wall -Wextra -Wpedantic -Werror`, separate fresh `-O1` and `-Og`
compilations, `-g -fsanitize=address,undefined -fno-omit-frame-pointer`, non-PIE
linking and `-D__NO_MYSQL__`. Runtime used
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

| Fresh profile | Cases | Input pins | ELF SHA256 | Artifact-manifest SHA256 |
| --- | --- | --- | --- | --- |
| `acceptance-v2-O1` | 49/49 PASS | 521 | `2dcd4db546944cbbb77cd285b7bfad642b09481a97e9e669c1a4cadfbde654b1` | `b1f5674fe4447809ab1f86785453fbfefd08ac14536e22b8b8e05b7fe4b92503` |
| `acceptance-v2-Og` | 49/49 PASS | 521 | `4fb9b0abcdf83973cd8952e7103d0fdb69e655d617c4e538fed2a97e33a8372a` | `fd7cb5d7e2b28a6b2063944af69f52994321e729c4f1acaf407e9e55c08820c4` |

The final script SHA256 is `059c52690344659bbfb77ed6b2a51416bfb64de24e9f15060648f261af80cbe5`; its Git blob is `51c1e091c89c5d063a396d730451bcd7fa9e37a4`.
Both generated harnesses are SHA256 `579bd235ff056297109c41213687a443369b8d95dbfa671eccfa898a2689cb7e`. The original existing
`test_auction_ownership_publication.py` remains blob
`22e4f597390d5161f3ae7a4aacedf5dd46d41779`, SHA256 `463509432c746d91df41dc514bbdca8851beff7b8704bbeff1fc611f69ded8ac`,
unchanged and neither imported nor executed by the new test.

Retained private evidence is under
`D:\Dev\Temp\auction-world-observation-20261008` and
`D:\Dev\Builds\Duris\auction-world-observation-20261008\bin`.
`qualification.json` records exact source identity, source-export verification,
test/harness pins and both results. Its SHA256 is `a25bb742b5d46706e36ce23c24d45db37f09e43b4d9e99705a78a88a583c01dd`.
The final `evidence-index.json` is SHA256 `fbfe9623c56546f1c05227891a3a8b1cd538b38f249410653cc5c49f3777ade0` and authenticates
203 retained final evidence files. It is an index, not a claim
that an independent coordinator review has occurred.

For each final profile the runner retains the generated C++, compiler version
and executable pin, all twelve compiler commands/logs/objects/dependency files,
link command/log/map and ELF, input SHA256 pins including GCC `-MD` transitive
project/system headers, `ldd` output and resolved shared-library SHA256 pins,
49 per-case transcripts, return codes and the artifact manifest. Input hashes
are verified again after execution. Discovery/development evidence is separate;
the earlier 45-case development run is superseded by these final 49-case runs.

Both final profiles put build outputs and compiler temporary files on the task's
D: paths. The generated fixture also passes the repository clang-format check.
No full server build was required: no production C/C++ changed, and all eleven
selected production translation units were compiled in both focused profiles.
No full burn-in, CI wait, SQL/database operation, server boot, gameplay login or
save/recovery qualification was performed.

## Reproduction and interpretation

Export `src` and `tests/async` from exact primary `cda8aa6f...`, verify the tree
pins above, and copy the delivered test into that export. The following is the
executed WSL recipe; use fresh output directories for subsequent invocations:

```bash
export CXX=g++-12
export TMPDIR=/mnt/d/Dev/Temp/auction-world-observation-20261008/compiler-temp
export BIN_ROOT=/mnt/d/Dev/Builds/Duris/auction-world-observation-20261008/bin
python3 /mnt/d/Dev/Temp/auction-world-observation-20261008/candidate/tests/async/test_auction_native_world_observation.py --optimization O1 --build-dir "$BIN_ROOT/acceptance-v2-O1"
python3 /mnt/d/Dev/Temp/auction-world-observation-20261008/candidate/tests/async/test_auction_native_world_observation.py --optimization Og --build-dir "$BIN_ROOT/acceptance-v2-Og"
```

`--source-root` selects a separately exported tree when the test lives elsewhere.
`BIN_ROOT` supplies the retained output parent if `--build-dir` is omitted. The
runner refuses an existing output directory, retains failures, and does not
delete or replace prior evidence. Runtime controls can also be independently
repeated by invoking a retained ELF with each case in the script's `CASES` tuple
and the sanitizer environment above; authenticate the ELF/inputs first.

This is client-free component acceptance for observer/capture behavior. It does
not establish SQL authority, transactional auction semantics, held publication,
native allocation/placement, NAR/ACK, current-save proof, cold restore, packet
authentication, activation or recovery. SQL-client builds reject flat-file
primary under the maintained policy; supported client-free builds select the
genuine flat-file worker/replay. This test does not invent an SQL-enabled
flat-file requirement, and it does not execute either worker policy branch.

Independent coordinator review and adoption remain separate. The continuing
native Goal remains blocked and unfinished; this finite additive acceptance
delivery does not resume, complete, or unblock it.

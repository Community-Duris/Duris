# Original room seed closure and UID-owner handoff - 2026-10-08

A fresh isolated proposal establishes why the original seed still cannot link:
adding its 21 first-level real providers exposes 22 transitive unresolved functions
and a duplicate native UID counter definition. No eligible seed, SQL service,
cold boot or corrected closure is produced. Full Plan5/R1-R8 remains open.
This handoff narrows a primary-owned fixture/recipe repair; it does not substitute
passing components for the original recovery gate.

## Branch, exact source and owned files

- Local/remote branch: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base: `6f6266f77d8b6ba86ef27ce2015888322221b6b2`.
- Publication result and verified remote: exact SHA in
  `D:/Dev/Tests/Duris/accounting-plan5/room-seed-closure-20261008/delivery/result.json`.
- Refreshed primary: `0588f178c64d88668b86f2889ea0e77937f0b4df`.
- Frozen primary plus 18 explicit owned overlays: `13c71b0f5fe5b92fca428a7ba13e5bc1751844c0`.
- Archive `candidate01.tar`, SHA256 `4890e6424c00d6fbb116f984aca5881514df92303bd8bf4eea0f7d6aad58fe0d`.
- Native tree `833d3085815b396861ad18a77635412212381e4b`.
- Canonical64 migrations tree `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.

All original body/mode/link guards pass before and after the attempted compile.
`source01.json` records all source inputs and overlay blobs. All seven earlier
branch tips remain required ancestors; all earlier work/follow-ups continue on
this remote. The owned branch intentionally retains older shared source. Shared
provider tracing uses the frozen primary tree and its actual compiled symbols,
not an assumption that worktree shared files equal the tested composition.
Earlier 0055-only evidence is not substituted for this source.

Only this report and the additive `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md` change in
the repository. Shared recipes, original harnesses, coordinator, producers,
contracts, registry/matrix, schema and activation source are untouched. No API,
DB field, wire format or schema change is requested.

## Exact proposal and observed failure

The previous handoff
`PLAN5_ROOM_OBSERVER_BUILD_AND_SEED_HANDOFF_2026-10-08.md` mapped 41 unresolved
symbols to 21 first-level maintained providers, without claiming a full closure.
This attempt tests that precise proposal while leaving the shared file untouched.
An external observer intercepts the original `compile_family.execute` call and
inserts those 21 actual providers before the existing garbage-collection linker
flag. Removing only those inserted source arguments reconstructs the original
compiler argv exactly. No defines, warning/sanitizer flags, wrappers, original
fixture bodies, assertions, source decisions or timeouts change.

The actual invocation from the owned worktree was:

```powershell
python -B D:/Dev/Temp/accounting-plan5-room-seed-closure/launch.py proposed01
```

Inside the frozen source it invokes the unchanged
`run_economic_sql_real_pool_mysql.compile_family("room", binary, timeout=600)`.
`proposed01/proposal.json` preserves both complete original and actual argv,
21 added source hashes and the original 600-second budget. The outer predeclared
budget is 660 seconds. Strict C++20, `-Wall -Wextra -Wpedantic -Werror`, pthread,
ASan/UBSan, non-PIE, function/data sections, garbage collection and all original
real-pool/MySQL wrappers remain. The original input map covers original sources
and all headers; the external proposal separately binds every added provider and
the complete frozen source. No qualification metadata/executable is issued after
failure, so an incomplete recipe input map is never presented as a qualified one.

Actual outcome: exit1 in 286.580s, native linker error. Both
`next_obj_uid` and its `__odr_asan.next_obj_uid` collide. Exactly 22 distinct
unresolved functions map to 11 further real maintained providers. The original
compiler driver cleans temporary objects/failed output; no eligible executable
remains, and `terminal.json` records an empty artifact map. The complete compiler
log, traceback, command, source archive and symbols are retained. No service,
SQL migration, runtime fixture or cold boot is attempted.

The explicit storage conflict is:

| Definition | Field and invariant |
| --- | --- |
| Original `tests/async/item_transfer_mysql_harness.cpp:93` | `unsigned long next_obj_uid = 1` |
| Actual `src/world/db.c:220` | `unsigned long next_obj_uid = 1` |

The original room harness includes the item harness. `db.c` is the actual provider
of `find_recovery_object_template(int)` required by the current retained paths.
Its native UID counter conflicts with the existing fixture counter. ASan's ODR
collision confirms two storage definitions; disabling the detector or permitting
multiple definitions would not prove the required single UID authority.

Further actual provider sources, established from all 754 maintained production
objects at the same native tree (not yet a corrected fixture closure):

- `src/economy/auction_native_publication.c`
- `src/economy/economic_baseline_codec.c`
- `src/economy/economic_baseline_command.c`
- `src/economy/native_mobile_birth_accounting.c`
- `src/economy/native_mobile_birth_constructor_recipe.c`
- `src/economy/native_mobile_birth_recipe.c`
- `src/persistence/economic_sql_auction_claim_endpoint.c`
- `src/persistence/economic_sql_baseline_transaction.c`
- `src/persistence/economic_sql_pending_claim_source.c`
- `src/player/player_save_worker.c`
- `src/world/native_quest_recovery_context.c`

Full unresolved signatures:

```text
auction_native_expected_player_forest(auction_command_payload const&, std::span<player_item_snapshot const, 18446744073709551615ul>, std::span<player_item_snapshot const, 18446744073709551615ul>, bool, unsigned int, std::vector<player_item_snapshot, std::allocator<player_item_snapshot> >*)
auction_native_selected_forest_valid(auction_command_payload const&, std::span<player_item_snapshot const, 18446744073709551615ul>)
economic_baseline_command_build(economic_prepared_baseline const&, unsigned long, critical_command*)
economic_baseline_decode(std::span<unsigned char const, 18446744073709551615ul>, std::optional<economic_prepared_baseline>*)
economic_sql_baseline_verify_known_retained_in_transaction(st_mysql*, critical_operation_id const&, economic_baseline_batch*)
economic_sql_baseline_verify_retained_in_transaction(st_mysql*, critical_command const&, unsigned long*)
economic_sql_pending_claim_endpoint_create(st_mysql*, critical_command const&, unsigned int, economic_account_key*)
economic_sql_pending_claim_endpoint_lock_absent(st_mysql*, critical_command const&, unsigned int)
economic_sql_pending_claim_endpoint_verify_retained_creator(st_mysql*, critical_operation_id const&, economic_account_key const&, unsigned int)
economic_sql_pending_claim_source_consume(st_mysql*, critical_operation_id const&, economic_account_key const&, unsigned int, unsigned long, unsigned long)
economic_sql_pending_claim_source_remaining(st_mysql*, economic_account_key const&, unsigned int, std::vector<economic_sql_pending_claim_remaining, std::allocator<economic_sql_pending_claim_remaining> >*)
economic_sql_pending_claim_source_stage(st_mysql*, critical_operation_id const&, unsigned short, economic_account_key const&, unsigned int, unsigned long)
native_mobile_birth_accounting_compile(critical_command const&, economic_account_key const&, economic_accounting_plan*)
native_mobile_birth_constructor_recipe_decode(std::span<unsigned char const, 18446744073709551615ul>, quest_mobile_native_constructor_recipe*)
native_mobile_birth_constructor_recipe_encode_blob(quest_mobile_native_constructor_recipe const&, std::vector<unsigned char, std::allocator<unsigned char> >*)
native_mobile_birth_constructor_recipe_valid(quest_mobile_native_constructor_recipe const&)
native_mobile_birth_recipe_decode(std::span<unsigned char const, 18446744073709551615ul>, std::span<player_item_snapshot const, 18446744073709551615ul>, std::vector<native_mobile_birth_item_recipe, std::allocator<native_mobile_birth_item_recipe> >*)
native_mobile_birth_recipe_encode(std::span<player_item_snapshot const, 18446744073709551615ul>, std::span<native_mobile_birth_item_recipe const, 18446744073709551615ul>, std::vector<unsigned char, std::allocator<unsigned char> >*)
native_quest_recovery_context_decode(critical_command const&, std::span<unsigned char const, 18446744073709551615ul>, native_quest_recovery_context*)
native_quest_recovery_fee_ack_context_valid(critical_native_recovery_envelope const&, critical_completion const&)
native_quest_recovery_publication_context_valid(critical_native_recovery_envelope const&, critical_completion const&)
player_save_worker_pid_pending(int)
```

`transitive-provider-handoff.json` binds every signature to its exact original
object and all source paths, plus both duplicate symbols. Every signature has a
real maintained match. Original full raw nm output is retained here and verifies
SHA256 `8bc52d69c6852f9a301653c94679093f0c16ba21e8828c430bdc6ad085172644`.
The source/symbol map identifies real providers; it does not claim that another
blind append would supply a complete closure or a valid fixture owner boundary.

## Concrete primary-owned repair request

1. Select one actual `next_obj_uid` storage definition for every item/room fixture
   mode that links the real world-template owner. Preserve its unsigned-long
   type, initial value, monotonic UID semantics and single address. Standalone
   fixture modes must retain their original ownership and assertions. The primary
   owns the original harness/provider boundary; no independent shared edit here.
2. Close the original maintained sources using real implementations, including
   the exact transitive bindings above. Do not replace unavailable native owners
   with invented stubs, rename away the storage conflict or weaken original
   unavailable-state/refusal assertions. Preserve original compiler wrappers,
   sanitizers, flags, fixture decisions and compile/runtime budgets.
3. `compile_family.inputs` must include hashes of every actual added translation
   unit and included harness/header dependency. Its `flags` must be the actual
   executed argv; executable destination and SHA must remain immutable. No DB or
   runtime interface change is needed for this maintenance request.

Consumers: original real-pool item/room families, optional retained room verifier,
and `run_sql_room_item_payload_recovery_journey.py`. Required tests after the
owner/closure repair: original item and room compile modes, retained-public-verifier
variant and original complete default room journey against fresh canonical64
MariaDB and MySQL, with two cold boots each. Preserve exact ACK/export ordering,
coordinator/pool shutdown, historical inactive transition, original graph bytes,
native UID/custody/revision equality, complete world and unchanged durable state.
No active-drop/global activation variant is run or claimed here.

## Verified observer/world reuse boundary

A separate read-only dependency proof shows that the earlier complete observer
binary and generated world are source-eligible for reuse once a seed is qualified:
all 1318 repository dependencies, all
2645 archived area/lib/root-Makefile inputs and the
native/migration trees match this frozen composition. The earlier world and full
server commands individually passed; its composite stage exit1 remains correctly
recorded because the earlier seed failed. That nonexistent seed is never reused.

The actual observer SHA256 is
`1ea8f0d1b1885b9272ac417dba2ed58706a80d4e0cf947312d5fde4bd6ce68bf`.
Its body and every generated world artifact are rehashed against the old sealed
record. `reuse-source-comparison.json` preserves those proofs and the original
seal SHA256 `3fe6f4a440aad9afc1a81c578566c5ec27a5dcf0da5a4c6b56a6e68fe59904b1`.
All 28 external dependency hashes and toolchain must also be checked in any
actual reuse container. This slice does not claim a new build, boot or runtime
pass merely from input equality.

## Evidence, curator workflow and remaining gates

Evidence: `D:/Dev/Tests/Duris/accounting-plan5/room-seed-closure-20261008/`.
Builds: `D:/Dev/Builds/Duris/accounting-plan5-room-seed-closure-20261008/`.
Helpers: `D:/Dev/Temp/accounting-plan5-room-seed-closure/`.
Image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`;
network none, two CPUs, 5GiB memory, source/tmp RAM and direct D: build mounts.
GCC13.3/Python3.12.3/nm2.42/mysql_config10.11.14 executable versions/hashes are
in `seal/native-build-inventory.json`. This is a SQL-client native compile only;
no MySQL/MariaDB database process or backend runtime was started.

Raw seal `seal/evidence.json`, SHA256 `13da2fb8fae62a421f98cc1c675da1db6f9f7a64474d3576873bd1844ebbe1cf`:
30 files/299316519 bytes, no copied links/reparse points.
Both the failed native container and read-only sealer are stopped. Post-push
delivery rehashes every sealed file, checks the exact remote/clean worktree,
18 overlay blobs and all seven preserved ancestors. No terminal stage is restarted
or overwritten and no unrelated checkout/build/service is modified.

This report plus the additive remote follow-up, raw seal and delivery receipt is
the curator packet for the primary's locally maintained shared notebook. User
explicitly made that notebook nonblocking. Application/acknowledgement/adoption
and separate cross-chat notification are not claimed.

The modern room audit and restore structural fixes remain separately qualified
at their exact sources in `PLAN5_SQL_ROOM_ITEM_CUSTODY_2026-10-08.md` and
`PLAN5_SQL_ROOM_RESTORE_EVIDENCE_2026-10-08.md`. Their modeled roots and passing
components do not close this original native room gate. Original cold boots,
full retained original payload/runtime authentication, genuine producer/
publication/ACK, complete census/retention/backup/upgrades/major-plan and combined
R1-R8 release qualification remain required. The primary private119 candidate
remains a separately owned, unexecuted source composition. Accounting inactive,
wallet-root ITEM_MONEY exclusions and declined inactive spell path remain.
No production data, activation, deployment, PR merge or audit autocorrection.

Independent work remains available: the original SQL/flat retention journeys
need qualification on current native/canonical64 rather than historical canonical61.
The earlier native fixture provider closure and all already-delivered claim
controls must be carried forward on this same remote branch.

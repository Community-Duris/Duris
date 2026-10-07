# Accounting domain separation handoff — 2026-10-07

## Current delivery status

**Continuation milestone active.** The completed Collector delivery below is
preserved. The user-authorized charter at accounting
`a7b3181edb80bf188f616c39b8e7cb144e11cb18` supersedes its first-delivery-only
completion boundary. See [the fixed operation inventory](OPERATION_INVENTORY.md)
for assessed domains and the fixed R0/R1/R2 delivery set. The coordinator accepted
inventory v1 and R1's seam in upstream review `b876f9442040653cf53c15d81d5188166e1cd829`.
R1 is implemented at `48cdf9cb0893873651216f7940aae2691d060e58`, with successful
focused components, both maintained builds and the real flatfile recipe journey.
Its SQL journey has also passed. R2 is implemented at
`24fa551ae16900b41e509f79fe4685762e0fb2e9`; original/extracted numeric controls and
both maintained builds pass. The selected production-linked completion subset
passes all 84 scenarios (42 per backend). Retained failures and private fixture
adaptations appear below. Final reviews/dispositions are being reconciled; the
continuation Goal remains active at this publication.

Actual continuation-tool evidence: `get_goal` first returned no current Goal;
`create_goal` then `get_goal` returned `status: active`, no budget, for this chat.
Objective: deliver the charter's bounded milestone by assessing/publishing the
fixed inventory, implementing/connecting/verifying required feasible extractions,
and publishing usable commits/handoffs on this existing branch while preserving
authority and treating required owner-blocked work as unfinished. Verified clean
checkout and branch at `ce3b631003303ee4fbfb8a0bd527ad8508982250` in the original
ac24 worktree. The Goal must not complete at the inventory or next-bundle boundary.

**R0 implemented and qualified at source/component/maintained-build scope**, on
`origin/codex/accounting-domain-separation`. Both maintained backends have
terminal successful builds. The bounded first delivery is ready for primary
review/import; upstream integration and actual Collector runtime journeys are
not claimed. Earlier proposed and implementation sections retain the research
and ownership history, not an outstanding implementation requirement.

## R1 published implementation and terminal evidence

Source commit `48cdf9cb0893873651216f7940aae2691d060e58`, parent
`92871c3dcb9bf90932eb102775ed9547022f4163`, published on the same owned branch.
`crafting_plan.h` contains the existing plan type and material-count rules with
owned value/VNUM/magical/multiplier inputs. `crafting_build_plan` captures the
native facts and calls that implementation. Preview and make retain their shared
wrapper. Native input selection/submission, output UID admission, progression,
recipe persistence and active guards are unchanged.

Terminal focused checks passed: `test_crafting_material_bounds.py` (original
ASan/UBSan/float-cast-overflow controls retained, plus direct owned cases),
`test_crafting_module_contract.py`, `test_crafting_config_contract.py`,
`test_crafting_recipe_persistence_contract.py`,
`test_crafting_enhancement_regressions.py`,
`test_crafting_material_probe_cleanup.py`, `test_recipe_craft_transaction.py`,
and `test_craft_progression.py`. The latter two compile actual native owner slices;
they do not replace gameplay/persistence journeys. Staged changed-line format,
full touched-header/source clang-format check and `git diff --check` passed.

Maintained development builds in task-owned container
`duris-domain-separation-ac24-r1` both finished with exit 0. Image
`duris-finish-accounting-qa:local`, ID
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`,
GCC 13.3, actual checkout read-only at `/workspace`; task-owned output volume
`duris-domain-separation-ac24-build` at `/workspace/bin`. These are dependency
rebuilds from qualified R0 objects, not claims of another 740-file clean build.

- `make -C src -j2`: SQL executable SHA-256
  `b85d8bbbd7970cf7fc831959531841658319a826c10df194bcec5fd80f0b72ab`.
- `make -C src -j2 PERSISTENCE_BACKEND=flatfile DMS_BINARY=/workspace/bin/server/dms_flat_new`:
  flat executable SHA-256
  `79e385143b87bf6d013f2e9c4a9d8393ee8359bbf71894076ed4370083872843`.

Source SHA-256 pins: `crafting_plan.h`
`623cd76374693dc87a940db5409fae4884313d1fa5affc401f68a8f6437cb268`;
`crafting.h` `ea148ac7f1276c3a81449339494df0bbd94ad2a91d66f205b3ead59066c98c00`;
`crafting.c` `cadb63d7c3d060ed1e4628df21624f54d3fdb6bd70480c2e2dfa5cc766a96051`;
`test_crafting_material_bounds.py`
`6bc12e410bc7898399276548fc60033ff2f1ad8f773ab1d5d5c3793233a3d2bd`.

Private evidence under `bin/tests/domain-separation-r1-20261007/`: full SQL/flat
compile/link logs, terminal exits/hashes, component logs and source pins. No
artifacts, logs or player/database data are committed. Coordinator source review
at this exact code revision found no actionable defect; its independently run
unchanged sanitizer regression passed. Published review on accounting is
`d91f59af06239a5736d10498b89091699c5e05c6` (documentation-only since this source
base). Review/import and runtime qualification are distinct statuses.

Runtime qualification uses a separate owned container
`duris-domain-separation-ac24-r1-runtime` with no network, disposable fixture
account/ports/data and maintained binaries. The unmodified inspector attempt
failed at link with missing `economic_baseline_decode`; retained
`inspector-original.log`. A private launcher adds only the existing
`economic_baseline_codec.c` provider to the original inspector source list and
runs the unchanged `run_alchemist_crafting_journey.py` recipe-only assertions.
No shared manifest repair is made. This attempt is running at this publication;
superseded by the terminal flat journey below. SQL journey later passed as recorded below; primary import remains unknown.

## R1 terminal flatfile runtime qualification

The original inspector link failure is retained; the first private attempt added
`economic_baseline_codec.c` and exposed its required existing
`economic_baseline_adapter.c`. Both failures remain in `inspector-original.log`
and `recipe-flat.log`. The final private launcher adds both production providers
without changing shared runners, gameplay assertions, fixture account, or
timeouts. `recipe-flat-with-adapter.log` finished `RECIPE_FLAT_EXIT=0` against the
R1 flat executable `79e385143b87bf6d013f2e9c4a9d8393ee8359bbf71894076ed4370083872843`.
The unchanged recipe-only journey copies that binary into its own runtime before
booting, so later R2 builds do not alter it. It passes actual preview/material
requirements, mortal Craft and Forge exact material/tool retirement, fresh output
UIDs and XP, retained pouch counters, copyover and two cold restarts.

An SQL recipe-only run uses the retained R1 SQL executable, unique disposable
schema on a task-owned MariaDB server at container loopback port 33306, canonical
bootstrap/migration runner and existing cleanup. Container has no external
network. SQL result later passed as recorded below. No production database,
real user account or upstream activation is used.

## R2 implementation and terminal build checkpoint

Reviewed reservation [R2_RESERVATION.md](R2_RESERVATION.md), commit `82ef254e0`;
source commit `24fa551ae16900b41e509f79fe4685762e0fb2e9`. Only the three local
helper bodies, new owned header and focused new regression change. The native
wrappers capture `std::array<int,4>` and call the owned value/payment rules.
Positive reward decomposition retains its actor-independent path. Spend refusal,
ascending bank denomination usage, change and success-only outputs are preserved.
All submission/identity/revision/admission/accounting/publication owners and
locker receipt/lore paths are unchanged. Coordinator reviewed the exact source
and independently passed original/extracted numerical controls with no defect.

Current-balance inputs deliberately have the native signed int range. Four such
counts times 1/10/100/1000 fit in int64_t. The original upper-overflow guard never
refuses defined native sums; its subtraction has undefined behavior when a
running total is negative. The extraction sums the bounded counts directly and
does not add validity policy or change supported nonnegative balance behavior.
Bank costs exceeding the bounded total refuse before ceil; even signed native
usage can move remaining by only bounded denomination values, so adding 999
cannot overflow int64_t. Native mutation still validates actual balances.

`test_currency_value_plan.py` passed under ASan/UBSan against the retained parent
helper preimage and final production wrapper/header with the same expectations,
flags and deadlines. An initial fixture type-name compile mistake is retained in
`value-extracted.log`; corrected/final runs pass. Exact source SHA-256 pins:

- `currency_value_plan.h`: `a043b439dd46a25974268f5a36ca77d553fb561760afe0a3d8af48977afb295f`.
- `currency_transaction.c`: `075a07dc357c11ea2c8f2fe87c02333c8213c37a3ea7d8424838f4a50dda90d7`.
- `test_currency_value_plan.py`: `e050407ceb90048bf6ae547e9b41354673664e17e1b5ef9d094a6bc5cbf5544b`.

Both maintained development builds completed with exit 0 in the same task-owned
runtime container/image and isolated output volume described above, using R1
qualified objects and full maintained links:

- `make -C src -j2`, SQL SHA-256
  `fad42d11f52857a6ee79f9af28f4c2ab73f34f84050476576a1bd3b56866f2f5`.
- `make -C src -j2 PERSISTENCE_BACKEND=flatfile DMS_BINARY=/workspace/bin/server/dms_flat_new`,
  flat SHA-256 `17aa1f48f932fcfb460817b66d0fffb5164a6c4661df23661bc2cedba2749ccd`.

Full logs retain both successful links. The original terminal script's final
flat hash command had a trailing CR and failed after the successful flat build;
`binary-pins.txt` records the corrected direct hash read. No repeated build or
altered compiler controls are claimed. Format check and `git diff --check` pass.
Private R2 evidence is under `bin/tests/domain-separation-r2-20261007/`.

Adjacent unchanged `test_locker_identify.py` passes actual lore/service controls.
`test_currency_transaction_contract.py` has 8 passing tests, 1 stale source-shape
error in untouched `critical_command_repository.c`, and 1 writer-census failure
for 12 existing assignments in untouched `coin_physical_recovery.c`.
`test_economic_currency_adapter.py` cannot link because its shared source list
omits `shop_trade_recovery_manifest.c`; a private existing-provider-only run later passed for both backends. Neither shared test is changed by this bundle.

The unchanged full `test_currency_completion_retention.py` fails link before
executing any scenarios: missing existing native-birth/Collector/recovery
providers and two newly referenced coin live endpoints. `completion-original-manifest.log`
retains it. A private launcher selects the original wallet/bank/identify scenarios
(excludes names containing `coin`), adds existing production providers and places
`abort()` guards on both excluded coin endpoints. Original selected assertions,
compiler sanitizers and deadlines remain intact. It later passed the selected 84 scenarios below. This is not a full-runner
pass or shared coin qualification. The first private provider attempt's additional
native codec/recipe link failures remain in `completion-selected.log`.

## Terminal R1 SQL and R2 caller qualification

`recipe-sql.log` completed `RECIPE_SQL_EXIT=0`, against the retained R1 SQL binary
`b85d8bbbd7970cf7fc831959531841658319a826c10df194bcec5fd80f0b72ab`.
It exercised the same original recipe-only assertions as flat: actual Craft/Forge
preview, exact materials/tools, fresh outputs, progression XP and retained pouch
counters, then copyover and two cold restarts with exact output UIDs and XP.
Canonical bootstrap, `migration_runner.py adopt --kind fresh_bootstrap`, then
`run` twice executed from this source tree. The runner removes its unique schema
in `finally`; subsequent `SHOW DATABASES` listed only system schemas, and no
journey server remained. No separately sampled schema-version number is claimed.

`completion-selected-providers.log` completed `COMPLETION_SELECTED_EXIT=0`:
**42 original selected scenarios per backend, 84 total**, actual production
currency transaction/codec/admission/authority adapter linked under the original
ASan/UBSan compiler controls. Both excluded live endpoints abort if reached;
none was reached. The original selected assertions and execution controls remain
intact; the original runner has 67 scenarios per backend, of which 25 coin-specific
scenarios per backend were deliberately excluded from this scoped evidence.

Added existing production providers only:
`quest_mobile_native.c`, `native_mobile_birth_recipe.c`,
`native_mobile_birth_constructor_recipe.c`, `native_mobile_birth_command.c`,
`collector_accounting.c`, `collector_command.c`, `collector_codec.c`,
`collector_policy.c`, and `shop_trade_recovery_manifest.c`.
Excluded endpoint guards:
`coin_physical_publication_restore_and_acknowledge(const critical_command &, const critical_completion &)`
and `player_save_pipeline_restore_sql_coin_obligation(const critical_command &)`.
No replacement result is returned from either guard. No shared manifest is edited.
These selected scenarios qualify real value/payment callers and their retained
publication contracts; they do not qualify native coin physical recovery or DB
transaction execution. Original shared runner link/contract defects remain owned
by the primary/shared fixture work and do not become extraction requirements.

Exact selected names, each run on SQL and flatfile:

- `platinum_active_persistent`.
- `platinum_active_local`.
- `platinum_refused_pickup`.
- `platinum_refused_review`.
- `platinum_success`.
- `platinum_failure`.
- `platinum_stale`.
- `platinum_replay`.
- `platinum_offline`.
- `platinum_awaiting_durability`.
- `platinum_uncertain_admission`.
- `platinum_ambiguous`.
- `platinum_malformed`.
- `platinum_local`.
- `malformed`.
- `already_applied`.
- `wallet_range`.
- `bank_range`.
- `ambiguous`.
- `ambiguous_with_payload`.
- `exhausted_retry`.
- `offline`.
- `rejected`.
- `rejected_without_payload`.
- `callback_chain`.
- `callback_rehash`.
- `active_rebasable`.
- `blocked_rebasable`.
- `accounted_bank_publication`.
- `accounted_bank_ack_retry`.
- `accounted_bank_invalid_result`.
- `accounted_bank_restart`.
- `accounted_bank_producer`.
- `accounted_bank_producer_restart`.
- `accounted_chaos_starter_producer`.
- `active_prepared_wallet_payment`.
- `active_prepared_bank_payment`.
- `active_prepare_wallet_payment`.
- `active_prepare_bank_payment`.
- `legacy_prepared_wallet_payment`.
- `legacy_prepared_bank_payment`.
- `active_pending_prepared_payment`.

Private `adapter-private.log` completed `ADAPTER_PRIVATE_EXIT=0` for both SQL and
flat sanitizer configurations, adding only the existing
`shop_trade_recovery_manifest.c` to the original currency adapter source list.
Its original typed bank transfers, starter supply and currency preparation
assertions are unchanged. It remains a private provider-corrected pass distinct
from the retained original runner link failure.

Terminal logs, source/binary pins and private launchers are copied into this
worktree's ignored `bin/tests/domain-separation-r1-20261007/` and
`bin/tests/domain-separation-r2-20261007/`. Proof file hashes are retained in the
private evidence index. No environment/credentials, game data, binaries or logs
are included in source commits. Both source bundles have independent coordinator
source/numeric review with no actionable defect; final published review evidence
and inventory dispositions must be reconciled before marking this Goal complete.

For primary import, review code commits independently: R0 `3d2b85b06`, R1
`48cdf9cb0893873651216f7940aae2691d060e58` (four files), R2
`24fa551ae16900b41e509f79fe4685762e0fb2e9` (three files). R1/R2 patches were
independently checked applicable to published accounting source. Confirm current
preimages and primary unpublished changes at import. Docs/inventory revisions are
handoff history, not a required execution dependency. Import/adoption, combined
candidate qualification and release/activation remain primary-owned and unknown.

## Goal and checkout evidence

Actual `create_goal` followed by `get_goal` returned `status: active` in chat
`01a11627-5fc2-7960-b5f1-f38e5183a822`. Objective: Implement and qualify the first
bounded domain-separation delivery for Community-Duris/Duris, preserving current
accounting behavior: select one stable domain, extract useful existing rules into
owned-state logic independent of SQL/live game pointers, integrate the production
caller, verify, and publish commits plus an integration handoff.

Separate managed checkout:
`C:\Users\alexa\.codex\worktrees\ac24\NewDuris Max`.
Initial clean detached HEAD and freshly fetched `origin/experimental-accounting`:
`7dbc472123a729f2fedc345e5309a586ba8a02d8`.
Verified no existing local owned branch and no checkout holding that name, then
created `codex/accounting-domain-separation` from this HEAD. All work uses this
checkout. Publication goes only to its same-named remote branch.

## Proposed first bundle — not primary-accepted

Selected domain: Collector of Antiquities. Selected operation: preparing the
selected collection item as an empty singleton, retaining its own weight and
literal properties while the existing command fences the complete source root.

Exact production boundary:

- New `src/economy/collector_collection_image.h`: owned `player_item_snapshot`
  plus explicit direct-child weights; validate and sum weights, calculate own
  weight, normalize singleton topology/equipment, encode through the existing
  snapshot codec, retain the existing collector blob bounds.
- `src/economy/collector_collection_preparation.c:capture_exact_blob` only:
  capture native objects and direct-child weights, invoke the separated rules.
  Both `collector_collection_prepare` and `collector_collection_live_matches`
  already use this helper; no second implementation of those rules remains.
- Extend `tests/async/collector_collection_preparation_harness.cpp` and its
  existing Python runner for owned inputs and real production codec evidence.
- This handoff is the only documentation change. No Makefile/manifest change is
  needed for the local header.

Research: collector purchase preparation and collector policy already use
owned snapshots, so repeating them would add no extraction. Auction/shop/coin
recovery/native birth/quest owners are actively changing in the published
primary handoffs and were excluded. Collection's preparation file last changed
at `4764ebf2e` (Collector of Antiquities lifecycle policy). The fetched Plan 5 tip
`45666a547` owns retained history/audit, and quest-prep tip `6db65f624` publishes
bartender attempt and native recipe fixes plus execution tooling. Their current
canonical handoffs were read, as were AGENTS.md, README.md, repository scopeguard
and plan-ablation skills, finish plan/fourth-agent contract, remaining requirements,
and primary shared producer progress. No AI_CONTEXT.md was present.

Overlap risk: the primary's collector publication/integration work may have
unpublished edits. This proposed boundary reserves nothing and requires no
primary acknowledgement. Compare the helper preimage before import; defer or
adapt the small local caller if it changed. No collector transaction/repository,
accounting adapter, audit, quest, shared codec or authority function is edited.

Semantics retained: direct children are released by the existing native owner;
the selected item's collected literal contains no children and has its own
weight. Preserve exact UID/VNUM, all literal fields, existing root/equipment
normalization, negative/overflow rejection, size bounds and failure mapping.
Time, randomness, listing/recipient/source identities, revisions and custody
claims are outside this calculation and remain untouched. Both admission and
publication compare the same bytes. The typed payload still carries the complete
source tree for native SQL proof and existing accounting adapters. An image is
not authority, effect proof, receipt or acknowledgement.

Verification planned: existing collection regression with real codec and owned
state controls, adjacent collector purchase/transaction/accounting regressions,
changed-line format check, maintained `make -C src` with unchanged flags, both
maintained backends if available. Component evidence will be distinguished from
actual database/server/player execution. No production DB use, migration,
activation, deployment or shared mutable outputs.

Import order: this research commit is advisory; the forthcoming implementation
bundle requires only a compatible accounting base containing the existing
collector helper/codec. Implementation, qualification and upstream integration
were pending at research publication; see subsequent bundle status below. Further
Collector rule extractions are future work and
outside this first Goal.

## Implemented bundle

Implementation commit: `3d2b85b0688684721f8db559cb3ea35b1830a1cc`. Its
parent is research commit `3e9d03db25ea459662b0cd6df8e764f05dd5a0e1`; accounting
base remains `7dbc472123a729f2fedc345e5309a586ba8a02d8`. Only the four proposed
production/test paths changed. No primary adoption or upstream integration is
claimed.

`collector_collection_prepare_image` now owns the original negative/overflow
checks, own-weight subtraction, singleton root/equipment normalization and
bounded production-codec encoding. Inputs are an owned item snapshot and a span
over explicit captured child-weight values, with no SQL or live object access.
`capture_exact_blob` captures those values and moves the snapshot into that
preparer. Native capture, eligibility, topology, custody and revisions still live
in the original adapter/owner. The new weight vector can fail allocation; its
catch retains the existing false/invalid-topology refusal of image preparation.
No frozen IDs, timestamps, recipients or revisions are recalculated here.

Production path: `collector_maintenance.c:process_candidate` calls
`collector_collection_prepare`, which builds the existing typed payload with
the image and complete source-root fence. `collector_transaction.c` calls
`collector_collection_live_matches` before its existing ledger/publication
effects and `collector_collection_detach_live`. Both preparation and comparison
call `capture_exact_blob`, hence the same extracted rules determine the bytes.
Existing SQL repository/accounting owners consume the same payload; locked
evidence, native effects, accounting references and durable boundaries remain
their responsibility. No independent transaction or accounting implementation
was added.

Executed so far: collection and policy runners pass. The collection runner now
links the real snapshot codec (section collection excludes unrelated codec
entrypoints); its original assertions/deadline remain. Added controls protect
identity/properties, child-weight rejection and overflow, zero/max weight,
codec failure, Collector blob limit, deterministic repeated preparation and
unchanged native fixture state. The existing room/corpse/root/claim/exclusion/
stale-publication assertions remain.

Retained adjacent failures: unmodified purchase-preparation, transaction and
purchase-accounting runners fail linking missing `shop_trade_recovery_forest_*`
symbols from their shared item-command source. Private diagnostic addition of
the existing `shop_trade_recovery_manifest.c` makes purchase preparation pass;
transaction exposes further missing native publication/authority dependencies.
These shared runners/owners are unchanged. Their failures are not passes.

Fresh maintained SQL then flatfile builds use a task-owned container
with this checkout mounted read-only at `/workspace`, task-owned volume mounted
as its `bin/`, no network, and no inherited objects. Compiler/dependency image:
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`,
Ubuntu 24.04 / GCC 13.3.0. No schema/backend runtime or player journey has run.
Terminal maintained results are recorded below.

## Terminal maintained builds

Both commands ran from `/workspace`, the read-only mount of this chat's actual
checkout, using the pinned dependency image above and a fresh task-owned `bin/`
volume. Default `BUILD_PROFILE=development` was retained; no compiler, warning,
hardening or feature flag was overridden. The flat binary override keeps the
SQL executable intact. No production-profile or runtime boot is claimed.

| Exact command | Terminal result |
| --- | --- |
| `make -C src -j2` | Exit 0; 740 fresh C++20 compiles and complete SQL server link. `bin/server/dms_new` SHA256: `6de48baed73542d44bf697ee93f4682b93f30e8c6d816ca940060cbe2c2659c1`. |
| `make -C src -j2 PERSISTENCE_BACKEND=flatfile DMS_BINARY=/workspace/bin/server/dms_flat_new` | Exit 0; 740 fresh C++20 compiles and complete no-MySQL server link. SHA256: `5b710436f11bae2b0691d6a65e1138f9d92a45ebd96128ee2cf863c936d2449a`. |

Private `build-sql.log` and `build-flat.log` retain complete original compile/link
commands; `build-terminal.txt` records `SQL_EXIT=0`, `FLAT_EXIT=0`, both hashes and
final make completion. The build container terminated with exit 0. Its isolated
volume retains the objects/binaries; completed evidence was also copied into
this checkout's local `bin/tests/domain-separation-20261007/`. `RESULT.json`
records terminal scope/results and authenticated source pins. Every changed
production/test file is byte-identical to implementation commit `3d2b85b06`.

## Final component evidence and import instructions

Import production/test commit `3d2b85b0688684721f8db559cb3ea35b1830a1cc` on a
compatible candidate. Research commit `3e9d03db25ea459662b0cd6df8e764f05dd5a0e1`
is advisory; `bc19aa682` records implementation progress. Import this canonical
handoff's subsequent final metadata commit as desired. No other stream's commit
is a prerequisite. Base preimage of collection source (SHA256):
`c57188914083bf4836930e7bd8d869e6e56d48b525a648771102b9d23ba60b35`.
Fresh upstream fetch during qualification still returned
`7dbc472123a729f2fedc345e5309a586ba8a02d8`; all selected existing paths matched
that base. Unpublished primary collector work remains an overlap risk.

Later fetched upstream documentation-only revision:
`27ba5cd565bbbff68feee7cde3e4574c6eaed3ca`. The
[independent coordinator review](https://github.com/Community-Duris/Duris/blob/27ba5cd565bbbff68feee7cde3e4574c6eaed3ca/docs/persistence/economy_accounting/domain-separation/COORDINATOR_REVIEW_2026-10-07.md)
reports no actionable source defect and independent passes of the published
collection/policy runners. Its observed build status is a checkpoint, superseded
by this handoff's terminal build results. The selected production/test preimages
still match the original base at that fetch. This branch imports no upstream
review or finish-plan edits and claims no primary adoption.

Production source pins (SHA256): image header
`50cc1048cf8cc622f3ce60eb802985e149671768a3e3fc310a95835a79c73c9d`;
collection adapter
`8bfbea5be229efdc030a90151f154634dedcb6a95d0ff111a8e82ff5373493f7`.
Test harness pin:
`0178471c74cd177d77064c78b8f7ff0f8958db1616be002d43e98f5aa68a4504`;
runner:
`0a04e9b253e136185b830322ff20b1bef161d453f0998583303ae86aa8af6fd1`.
Unchanged migration manifest source pin:
`06301322855e336898d76893c58955e63ae3fdcf0167dd005801723bc93a1162`.
This is a source pin; no schema was applied or database certified.

Private evidence root in this checkout: `bin/tests/domain-separation-20261007/`.
Build container: `duris-domain-separation-ac24`; isolated build volume:
`duris-domain-separation-ac24-build`, mounted as this checkout's `/workspace/bin`.
Source is the actual Windows worktree mounted read-only, not a copied alternate
checkout. No other agent's output directory, server, test database or account
was used. Evidence includes original failed logs, augmented logs, source pins,
frozen original source, sanitized executables and the private exact-command
verification scripts. Private artifacts are not committed.

Executed commands/results:

| Command | Result and evidence scope |
| --- | --- |
| `python3 tests/async/test_collector_collection_preparation.py` | PASS: initial component on WSL GCC11; final frozen harness on image GCC13.3, including all production-codec controls (`collection-final.log`). The independent coordinator separately passed the published final runner under WSL. Native object/custody/capture fixture seams remain; this is an executable component, not a player journey. |
| `python3 tests/async/test_collector_policy.py` | PASS; `policy-final.log`. Existing policy component. |
| `bash scripts/format.sh` then `bash scripts/format.sh --check` | PASS. WSL used explicit `GIT_DIR` pointing at this managed worktree's actual Git metadata and `GIT_WORK_TREE` pointing at this checkout because its `.git` contains a Windows absolute path. Final direct clang-format dry-run of all three touched C++ files also passes. |
| `git diff --check` / staged equivalent | PASS. |
| Original purchase-preparation / transaction / purchase-accounting Python runners | FAIL linking the missing shared recovery-manifest functions; original logs retained. None of these runners or their shared owners was edited. |
| Private `verify_components.py` | Original runner flags/assertions/deadlines, only existing `src/economy/shop_trade_recovery_manifest.c` added to compile commands: purchase preparation PASS; purchase accounting PASS (its original ASan/UBSan flags retained); transaction still FAIL on missing publication/authority symbols. See `purchase_preparation-augmented.log`, `purchase_accounting-augmented.log`, `transaction-augmented.log`. Diagnostic link completion does not make the unmodified runners pass. |
| Private `verify_images.py` | Original source and extracted source both PASS the same final collection assertions under ASan/UBSan; deliberately replacing own-weight subtraction with the unadjusted weight fails by assertion (return -6), proving detection. All source inputs are the owned checkout; frozen original is byte-identical to base. |

Private image verification compile commands use `g++ -std=c++20 -Wall -Wextra
-Wpedantic -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
-fno-pie -no-pie -ffunction-sections -fdata-sections -Isrc`, production
`collector_policy.c`, `player_snapshot_codec.c`, the final collection harness,
and original or extracted collection source, then `-Wl,--gc-sections -lcrypto`.
Environment: `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`; each execution retains the
30-second component deadline. Exact argument lists are in the respective logs.
The negative-control include override exists only under private evidence and
was not applied to production source.

Native Collector SQL/flatfile database, server/player, replay and recovery
journeys are **unrun**. This local image extraction changes no authority decision
or accounting route; codec/components and maintained builds establish only
that bounded seam's behavior. The original production command/transaction
owners, backend proofs and primary's combined-candidate qualification remain
required. Full accounting and actual RAM execution are outside this delivery.
Upstream-integrated status remains **pending/not claimed**.

The remaining transaction diagnostic link failures include
`collector_purchase_publication_attempt`,
`economic_gameplay_authority::prepare_collector_purchase`,
`collector_purchase_submit_for_publication`,
`player_save_pipeline_restore_sql_collector_purchase_obligation`,
`nevent_is_game_thread` and `persistence_mode_get`. They are shared test-link
closure deficiencies, not failed image assertions. The completed maintained
SQL link resolves the real production owners. No shared runner closure repair
is included in this bundle.

To reproduce each augmented diagnostic without editing its runner, run this
from the imported checkout under Linux (substitute the transaction or
purchase-accounting filename for the other two original cases):

```bash
python3 - tests/async/test_collector_purchase_preparation.py <<'PY'
import runpy
import subprocess
import sys

original_run = subprocess.run
def with_existing_manifest(command, *args, **kwargs):
    if isinstance(command, list) and command[0] == "g++":
        command = [command[0], "src/economy/shop_trade_recovery_manifest.c", *command[1:]]
    return original_run(command, *args, **kwargs)
subprocess.run = with_existing_manifest
runpy.run_path(sys.argv[1], run_name="__main__")
PY
```

To reproduce the positive original/extracted sanitizer image controls:

```bash
evidence=bin/tests/domain-separation-20261007
mkdir -p "$evidence"
git show 7dbc472123a729f2fedc345e5309a586ba8a02d8:src/economy/collector_collection_preparation.c > "$evidence/original-collection.c"
for variant in original extracted; do
    source_file="$evidence/original-collection.c"
    if [ "$variant" = extracted ]; then
        source_file=src/economy/collector_collection_preparation.c
    fi
    g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -g \
        -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie \
        -ffunction-sections -fdata-sections -Isrc \
        src/economy/collector_policy.c src/player/player_snapshot_codec.c \
        tests/async/collector_collection_preparation_harness.cpp "$source_file" \
        -Wl,--gc-sections -lcrypto -o "$evidence/$variant-sanitized" || exit
    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
        UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
        timeout 30 "$evidence/$variant-sanitized" || exit
done
```

The private negative control copies only the image header under
`$evidence/negative/economy/`, replaces
`static_cast<int64_t>(selected.weight) - children_weight` with
`static_cast<int64_t>(selected.weight)`, and prepends
`-I"$evidence/negative"` to the same extracted compilation. Execution must fail
its weight assertion. The private script and negative log retain that exact
substitution, compile command and return code.

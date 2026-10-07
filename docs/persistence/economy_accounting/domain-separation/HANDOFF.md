# Collector domain separation handoff — 2026-10-07

## Current delivery status

**Implemented and qualified at source/component/maintained-build scope**, on
`origin/codex/accounting-domain-separation`. Both maintained backends have
terminal successful builds. The bounded first delivery is ready for primary
review/import; upstream integration and actual Collector runtime journeys are
not claimed. Earlier proposed and implementation sections retain the research
and ownership history, not an outstanding implementation requirement.

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

# Accounting completion plan

This is the execution plan for the final stretch, consolidated on 2026-10-11
from the paused implementation handoff at
`11ea970dc616a04069cda703d3afe9cc02b4c7d1`. Maintained implementation is based on
`f215679503d787a25af52fa8103506159a50013d`. Accounting remains inactive,
admission CLOSED, coverage incomplete, and release BLOCKED.

The primary implementation agent is stopped. This documentation cleanup does
not resume implementation, dispatch helpers, or authorize activation. On a
user-authorized implementation resumption, use this finite queue and the
[R1–R8 requirements](REMAINING_REQUIREMENTS.md). Historical Plan 1 acceptance
retains its original scope; Plans 2–4, full Plan 5, required architecture work,
and combined release qualification remain incomplete.

## Preserved inputs and actual starting state

Read the [complete paused handoff](SHARED_IMPLEMENTATION_PAUSED_HANDOFF_2026-10-11.md)
before adopting either patch. It is the authoritative description of this
checkpoint's implementation and proof limits.

| Input | Actual state | Next use |
| --- | --- | --- |
| [29-file shared candidate](paused-handoff-20261011/shared-candidate.patch) and [fingerprints](paused-handoff-20261011/CANDIDATE.json) | Portable proposal, source/format reviewed; not applied to maintained source and not compilable because seven exports are missing | Reconcile owner deliveries, then integrate and compile |
| [Recovery runner patch](paused-handoff-20261011/recovery-runner.patch) | Adds three missing production providers to the existing fixture; no native batch executed | Independently review, rebind the recipe, and run the original regression |
| [Stopped recovery recipe](paused-handoff-20261011/recovery-recipe/ROOT-COMMANDS.md) | Historical HEAD, status and private-path guards; not directly runnable at the handoff revision | Preserve its checks while rebinding exact source and D: paths |
| [WORLD proposal](WORLD3_UNQUALIFIED_LEASE_HANDOFF_2026-10-11.md) and its three patches | Preserved unqualified owner proposal | Reconcile with the shared candidate rather than treating it as an installed interface |
| `writers.json` and `writer_coverage_matrix.json` | 931 writer policies, 2,911 census entries, 2,853 sites, 511 source pins reported by the paused handoff | Validate the current records and genuine source hashes; counts do not close coverage |
| Plan 5 tip `52ca6d9fba8da46bb63c1ce4d2da328e6bf6ab29` | Retention recipe adoption verified; full current retention/restore and release remain blocked | Resolve schema and independent verifier prerequisites |
| Local qualification branch `codex/accounting-accel-integration` | Divergent helper candidate, not integrated by the paused primary | Compare concrete repairs/evidence individually; do not merge the entire branch blindly |

The three excluded dirty files and their exact fingerprints are listed in the
paused handoff. Preserve them and every existing worktree, candidate packet,
failed run, and evidence volume. Local `tmp/` paths on the remote host are
historical provenance, not portable test inputs. The sibling knowledge overlay
is retained; this plan does not claim to update or verify that private notebook.

## Ownership on resumption

Use the latest direct authorization recorded in the paused primary handoff.
Its WORLD/SHOP leases supersede the earlier withdrawn-delegation text. Verify
each owner's actual branch, source and dirty work before transferring a file.
No owner is restarted by this plan.

| Owner | Files / responsibility | Required delivery |
| --- | --- | --- |
| Primary / successor | Shared Save, Collector, Redis, Coordinator, Currency, Authority, Zone; final integration, registry/matrix and activation implementation | One coherent candidate, real caller integration and final tested source |
| WORLD owner | `src/world/db.c`, `src/world/quest_mobile_native_birth.c/.h` | Three missing birth exports, exact Source/INITIAL/query contracts, proof for their real dependencies |
| SHOP owner | `src/economy/shop_trade_transaction.c/.h`, `src/sql/sql_player.c`, `src/flatfile/flatfile_item_repository.c/.h` | Four missing shop replay exports and genuine owner evidence |
| Independent Plan 5 owner | Read-only verifier/reconciler, operator tools, schema/restore/retention and independent release evidence | Existing verifier ABI, coherent current schema qualification, complete audit/lifecycle proof |

An unavailable owner creates an implementation dependency, not a requirement
to produce more coordination notes. Resolve a narrow ownership transfer with
the human user when necessary. Existing helpers are used only for an exact
bounded deliverable under current authorization, then returned to idle.

## Ordered completion queue

Before implementation, repair the three existing coverage-contract findings
reproduced at the untouched paused tip and at the cleanup candidate: the stale
bandage `tradeskill.c:1214` anchor, the selector matching two room-payload INSERT
sites, and the stale `tests/run_test_entry.py` source hash. The suite currently
passes 54 of 57 cases. Reconcile actual writer ownership/reachability and source
hashes in the existing tests/metadata; do not weaken coverage or silently bless
both sites. These findings were preserved by the cleanup, not introduced by it.

### 1. Close the independent recovery fixture dependency

This work does not depend on the seven missing shared interfaces. Review the
preserved runner patch and recipe first. Rebind source, clean-tree assertions,
compiler/image identity, input fingerprints and storage paths explicitly.
Keep the original fixture assertions and flags, 17 whole production translation
units, 900-second guest / 1,020-second host / 20-second fixture deadlines, and
offline 8 GiB / 2 CPU resource shape. Recheck available storage; an old free-space
sample is not a reservation or disk quota.

Use the existing selector:

```text
python3 tests/run_regression_tests.py --profile native --match native_quest_recovery_context --list
```

Then execute that registered test using its existing runner under the qualified
toolchain and task receipt. Publish the runner only after genuine native PASS,
with exact commands, exit results, source/backend and cleanup evidence. Add its
actual source hash to both `candidate_worktree_evidence.source_pins` maps only
when it is adopted: expected 511 to 512 entries, preserving all earlier entries.
This closes a fixture dependency, not full recovery or a release gate.

### 2. Complete the seven missing owner exports

The shared candidate requires these genuine definitions:

```text
quest_mobile_native_birth_current_initial_inline_bytes
quest_mobile_native_birth_current_source_profile_query_frames
quest_mobile_native_birth_current_initial_query_frames
shop_trade_transaction_replay_current_source_frame_bytes
shop_trade_transaction_replay_current_initial_inline_bytes
shop_trade_transaction_replay_current_source_query_frame_bytes
shop_trade_transaction_replay_current_initial_query_frame_bytes
```

Deliver exact base/result revisions, files, whole Source/INITIAL lifetimes,
numeric returns, getters and query-accessor execution unions including reached
dependencies. INITIAL must be explicitly disjoint from full Source; do not
infer zero or fabricate declarations/allowances. Birth already owns Warm.
The Redis-enabled fifteen-family census owns store CURRENT; the held cache
scope excludes overlap without charging that census again. Preserve NO_REDIS
refusal and unsupported-profile refusal.

Exit: all seven actual interfaces have compatible owner definitions and focused
native evidence, and the shared candidate has no unresolved export dependency.

### 3. Integrate one complete shared candidate and verify it

Apply the preserved candidate only after checking its original-base fingerprints
and reconciling newer owner changes. Maintain one Root inline owner, one Currency
metadata charge, and CURRENT admission before child metadata queries. Verify the
genuine execution holds, loans, locks and publication order; fresh publication
strings transfer after the last fallible admission check. Pending work must not
publish twice, and normal-return health refresh remains intact. Keep the original
publisher's unqualified allocation/throw boundary explicit until it is tested;
do not invent a catch-and-retry path.

Freeze the combined source once. Run changed-line formatting, full production
build/link for SQL and flatfile with `BIN_ROOT` on D:, supported/assertions and
applicable sanitizer/profile checks, original focused caller/refusal tests, and
reproducible inactive build/boot checkpoints. Qualify fresh Currency/ATM callers,
bounded Authority selection and the affected Save/Collector/Redis/Coordinator
paths using real production providers. Report components separately from actual
commands and persistence journeys.

Exit: the combined maintained candidate compiles, links, boots under its tested
configuration, and affected caller/protocol checks pass on the exact source.
Record failures before fixing them. Do not run a full matrix after every docs commit.

### 4. Finish independent initialization and current schema/lifecycle proof

Plan 5 must implement and qualify the genuine independent initialized-world
verifier through the existing `economic_sql_initialized_activation_verifier`
ABI. The primary supplies the original held session and exclusions and joins
verification between `complete_boot()` and `before_world_callbacks()`. Missing
proof or failed cleanup preserves refusal/abort obligations. A substitute
callback, separate session or new CLI does not satisfy this contract.

Reconcile original retention schema `0061` and restore `0062` preconditions with
the current default manifest ending at `0065`. Preserve historical tests at their
original schema scope and add/repair the actual current route as necessary;
changing an expected version without qualifying its behavior is insufficient.
Measure and authenticate real schema compatibility on disposable MySQL and
MariaDB fresh/upgrade fixtures. Run retention, managed restore, independent audit,
erasure/export and native authority checks against that coherent source/schema.

Exit: current initialized-world, retention and restore evidence comes from the
same qualified candidate, schema and authority context; required negative controls
fail safely and independent audit remains read-only. No production operation follows.

### 5. Finish required responsibility boundaries with functional closure

After the executable checkpoint, map actual owner/thread/lock, operation and
revision identity, admission, durable commit, live publication, checkpoint, ACK,
retirement and valid pending/uncertain/retry states across Save, Coordinator and
Item movement. Select the smallest connected extraction and verify it before
stacking another. The earlier held-retirement extraction is scoped existing work,
not proof that all architecture requirements are complete.

Required areas are the shared save lifecycle (`player_save_pipeline.c`), shared
operation lifecycle (`critical_command_coordinator.c`), item publication
(`item_movement_transaction.c`), world/reset/native-birth stages, shop preparation /
execution / cold recovery / publication, and actual SQL/flatfile storage boundaries.
Keep one mutation/recovery authority and atomic native effect/evidence/receipt
ownership. Retain authenticated private capabilities, wire/journal/schema semantics,
UID/source dedupe, operation identities and supported gameplay. Separate toolchain
storage profiling from gameplay policy and preserve verifiable caller budgets.

For each boundary record before/after source, responsibility, callers and risks;
run affected production builds and existing meaningful executable regressions for
refusal, uncertain commit, committed-but-unpublished state, repeated ACK/publication,
disconnect, shutdown/copyover and cold replay. Remove superseded implementations
after real callers move. Avoid empty forwarding layers, parallel authorities and
whole-project rewrites. Required architecture work accompanies functional closure.

### 6. Qualify supported writers and real gameplay end to end

Use the existing [five domain plans](DELIVERY_PLAN.md) and R1–R8 contract as the
acceptance crosswalk. Complete money transfers/holdings, sourced issuance/sinks,
coin piles, ordinary UID supply/custody and same-owner topology, then compound
shop/auction/Collector/craft/death/resurrection/restoration operations. Preserve
actual prices, fees, separate death/resurrection commit boundaries and source dedupe.

Day-1 supported gameplay must include sequential NPC item-give quests,
bartender quests, mob loot/coin issuance, reset spawns, remove-curse drops and
consumables with durable UIDs and zero unsupported refusals. Preserve the active
blackjack deprecation policy and already delivered scoped fixes. Unsupported
nonrequired routes must refuse before mutation in an active epoch.

For every supported route prove same-root native effects, balanced money,
exact UID/source evidence, once-only receipt/publication and independent read-back.
Exercise real player journeys, disconnect/reconnect, save, restart/copyover,
rollback, lost replies, ambiguous commit and cold recovery. SQL is the first
milestone; both SQL engines and flatfile parity remain required for full completion.
Refresh the semantic writer census without relabeling component evidence as coverage.

### 7. Freeze and certify the release candidate

Run the existing release validator, gameplay/fault matrix, schema checks,
backup/restore/retention/erasure/export and independent reconciliation on one final
frozen combined source. Measure actual operation size, latency, storage growth,
checkpoint/reconciliation workload and the complete 32 MiB host bound, including
transitive frames, ABI/allocator/library paths and overlapping retained lifetimes.
Neither guessed layouts nor isolated provider allowances establish this bound.

The report must identify source/binary/schema/image, backend, command, result,
skips/refusals, evidence location, coverage and resource release. R1–R8, all
required responsibility boundaries and required backend journeys must pass with
zero unclassified reachable writers and independently consistent money/UID state.
Technical completion does not authorize activation, PR merge, deployment,
production migrations, restitution or resolution of private historical cases.
Issue #490 remains the implementation/qualification tracker; #664's historical
case work retains its separate authorization and scope.

## Work discipline and progress recording

Use D:/Dev/Temp, D:/Dev/Cache, task-specific D:/Dev/Builds and D:/Dev/Tests paths.
Use `BIN_ROOT` and direct D: Docker mounts. Obtain the resource grant and exclusive
owner/job receipt before starting a container; inspect compatible slots/jobs first.
Use disposable foreground `--rm` jobs or stop/release owned reusable workers in
finally/trap cleanup. Stop a development game before its database via its documented
path. Preserve unrelated services, stopped recovery containers, volumes and evidence.

Record one concise row per solved task: source, owner/files, actual tests and
results, evidence link, remaining dependency. Put detailed evidence in the existing
dated report, not repeated copies in the plan, requirements and review checkpoint.
Source-only review, a helper's handoff, recipe adoption, green inventory or an older
binary cannot advance a release gate. Do not poll or repeat unchanged blocked work.

## Documentation and history consolidation

The [cleanup receipt](BRANCH_CLEANUP_2026-10-11.md) identifies the preserved
original history, replaced coordination accumulators, removed redundant commits
and validation. Older entry-point paths remain short compatibility links.
Original dated technical reports and all candidate/evidence payloads are retained
byte-for-byte; their proof remains tied to original source revisions. Use the
preserved history and commit map for original references, not rewritten SHAs as
new qualification. Adopt the cleanup only with an exact remote-head lease and
explicit coordination of stopped owner checkouts.

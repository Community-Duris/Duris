# Accounting completion plan: parallel delivery

Deliver one executable combined candidate, then finish the supported gameplay,
architecture and independent release gates in bounded parallel lanes. This plan
was prepared on 2026-10-11 and approved for execution on 2026-10-12 from cleanup source
`c8452833c5e2d4819341a144444dd9638c4fe78d` and the paused handoff at
`11ea970dc616a04069cda703d3afe9cc02b4c7d1`. The maintained implementation base
is `f215679503d787a25af52fa8103506159a50013d`.

The user approved preparing the 330-document archive and asked for a faster
multi-agent completion plan. Implementation resumes under the ownership and acceptance gates below. Accounting remains INACTIVE, admission CLOSED, coverage INCOMPLETE and
release BLOCKED. R1–R8, the [five domain plans](DELIVERY_PLAN.md), required
responsibility boundaries and existing product decisions remain required.
Historical Plan 1 acceptance retains only its recorded scope. Technical completion
does not authorize activation, production operations, deployment or PR merge.

## Starting facts and reusable inputs

- Read the [complete paused handoff](https://github.com/Community-Duris/Duris/blob/codex/accounting-handoff-context-20261012/docs/persistence/economy_accounting/SHARED_IMPLEMENTATION_PAUSED_HANDOFF_2026-10-11.md).
  Its [29-file shared patch](https://github.com/Community-Duris/Duris/blob/codex/accounting-handoff-context-20261012/docs/persistence/economy_accounting/paused-handoff-20261011/shared-candidate.patch) and
  [fingerprints](https://github.com/Community-Duris/Duris/blob/codex/accounting-handoff-context-20261012/docs/persistence/economy_accounting/paused-handoff-20261011/CANDIDATE.json) are proposals, not installed
  or compiled implementations. The patch excludes the WORLD/SHOP leased files.
- Birth, SHOP and ROOM now supply passive Source, INITIAL and query exports
  with focused supported/refusal profile controls. These interfaces close the
  metadata dependency; the full descendant/caller and nonempty runtime closure
  remains required before shared accounting qualification.
- Boot currently has the verification interval between `complete_boot()` and
  `before_world_callbacks()`; its genuine independent verifier invocation is missing.
- The [recovery runner patch](https://github.com/Community-Duris/Duris/blob/codex/accounting-handoff-context-20261012/docs/persistence/economy_accounting/paused-handoff-20261011/recovery-runner.patch) adds
  three omitted real providers to an existing fixture. It is independently runnable
  after deliberate rebind/review and does not depend on the seven exports.
- The starting coverage contract had 54/57 passing cases: a stale bandage anchor,
  ambiguous room-payload selector and stale `tests/run_test_entry.py` hash. The
  scoped passive package now passes all 57 source-contract checks; census and
  runtime/backend coverage remain incomplete, and release remains blocked.
  The missing-main adapter implementation was already fixed at `c72a56041`.
- Plan 5 `52ca6d9fba8da46bb63c1ce4d2da328e6bf6ab29` retains a failed full schema65
  matrix on `a60f5dcc...`, a later focused DIAG-02 pass, and prepared RETRY-03
  using native Linux MariaDB storage. No completed full retry is reported.
  The historical `.ibd` rename failure is not proof of a current migration-code defect.
- Original retention and restore runners pin schema61 and schema62; default ends
  at schema65. The original65 and nullable-default65 selectors are distinct.
  Preserve their historical meanings and qualify the genuine current route.
- Retain all dirty work, source packets, failed runs and original evidence. The
  handoff's three excluded files remain protected. Divergent helper branches are
  intake sources, not permission for a wholesale merge. The sibling notebook is
  retained; this plan does not claim to update or verify that private overlay.

## Four active owners, one integration writer

Use one integrator and three bounded delivery owners. The integrator is the
existing primary or an explicitly approved successor, never both. Reconcile
stopped-owner leases and exact branch/dirty state before resumption. Use the accepted four-owner team after source intake and exclusive file leases.

| Lane | Exclusive write set | First deliverable |
| --- | --- | --- |
| Integrator | Shared Save/worker, Coordinator, Item movement, Zone/shared budget, boot/lifecycle integration, `src/Makefile`, registration and writer metadata | Repair current acceptance bookkeeping; qualify independent recovery fixture; join one complete candidate |
| WORLD/Birth | `src/world/db.c`, `src/world/quest_mobile_native_birth.c/.h` | Three genuine birth exports and actual dependency/caller proof |
| SHOP/backend | `src/economy/shop_trade_transaction.c/.h`, `src/sql/sql_player.c`, `src/flatfile/flatfile_item_repository.c/.h` | Four genuine replay exports and Source/INITIAL/query proof |
| Independent Plan 5 | Existing read-only audit/operator/verifier/lifecycle scripts and explicitly inventoried fixtures; one genuine native verifier implementation through the existing ABI | Independent initialized-world correspondence plus coherent schema65/retention/restore acceptance |

The Plan 5 native verifier's exact implementation file is selected and exclusively
leased in task P0 after checking the existing native reader layout. Its shared
callback type, build registration, boot glue and mutation/lifecycle transaction
remain integrator-owned. Do not invent a second authority, new callback ABI or
parallel public permits. Required changes outside a lane's lease return as one
narrow contract request naming fields/functions, invariants, consumers and tests.

The integrator owns integration and the heavy-job queue as part of implementation.
A fifth permanent coordination/research agent is unnecessary. Source work can
proceed in all four lanes while a leased frozen-source test runs. An idle or
blocked lane returns one concrete dependency handoff and stops repeated polling.

## Wave 1: remove the executable-candidate blockers concurrently

```mermaid
flowchart LR
    P0["P0: freeze inputs, ownership and contracts"] --> I1["I1: bookkeeping + recovery fixture"]
    P0 --> W1["W1: Birth exports + dependencies"]
    P0 --> S1["S1: SHOP exports + dependencies"]
    P0 --> A1["A1: independent verifier"]
    P0 --> A2["A2: schema retry + lifecycle intake"]
    I1 --> M1["M1: combined executable checkpoint"]
    W1 --> M1
    S1 --> M1
    M1 --> M2["M2: same-session verifier + current lifecycle"]
    A1 --> M2
    A2 --> M2
    M2 --> M3["M3: supported domain journeys + architecture"]
    M3 --> M4["M4: frozen release qualification"]
```

P0 authenticates patch preimages and current source, compares reusable helper
deliveries individually, preserves dirty overlays and publishes the table above
with exact accepted files. Use one source pin and one finite task table. No broad
survey, new framework or repeated ownership acknowledgment cycle is needed.

| Task | Work and dependency | Exit evidence |
| --- | --- | --- |
| I1 | Repair the three current coverage findings without weakening classification; reconcile the already fixed main adapter's source hash. Review/rebind the recovery patch and [recipe](https://github.com/Community-Duris/Duris/blob/codex/accounting-handoff-context-20261012/docs/persistence/economy_accounting/paused-handoff-20261011/recovery-recipe/ROOT-COMMANDS.md). | Current coverage checks pass; actual unchanged recovery fixture passes; runner adopted and real pin added to both maps only after PASS |
| W1 | Reconcile preserved WORLD changes and implement three missing exports with genuine constructors, parsers, templates, strings/vectors and reached library/provider lifetimes. | Definitions/declarations, complete Source/INITIAL/query union, supported/refused profiles, strong outputs/cleanup, affected production and native checks |
| S1 | Implement four missing replay exports, preserving real pending/cold/private-stage ownership, original SQL/flat authority and disjoint Source/INITIAL. | Complete getter/query execution and returned values, overflow/refusal, retention/publication/replay checks; exact source and provider proof |
| A1 | Build the independent native verifier using raw held-session inputs and existing ABI; no primary report/count substitutes for reconstruction. | Genuine healthy and corrupted same-session correspondence tests, stale/session/resource/cleanup refusals, no accidental mutation/activation |
| A2 | Reconcile Plan 5 source adoption, prepared schema65 retry and current retention/restore route with a frozen candidate. | Unchanged full two-engine matrix completes on the commissioned profile; measured schema evidence and real current lifecycle acceptance |

W1, S1 and A1/A2 source preparation proceed concurrently with I1. A1 and A2
share the established independent owner; source work may continue while its
separate frozen-source matrix runs. A2 commissioning can use unchanged maintained
schema inputs before M1 if its full runner prerequisites are met; after M1,
qualify the final combined candidate wherever relevant source changes invalidate
earlier results. Do not rerun unchanged full matrices merely to produce fresh dates.

### Exact missing owner interfaces

WORLD:

```text
quest_mobile_native_birth_current_initial_inline_bytes
quest_mobile_native_birth_current_source_profile_query_frames
quest_mobile_native_birth_current_initial_query_frames
```

SHOP:

```text
shop_trade_transaction_replay_current_source_frame_bytes
shop_trade_transaction_replay_current_initial_inline_bytes
shop_trade_transaction_replay_current_source_query_frame_bytes
shop_trade_transaction_replay_current_initial_query_frame_bytes
```

The query profiles cover actual getter plus query-accessor execution, all reached
dependencies and numeric returns. INITIAL is explicitly disjoint from full Source;
do not infer zero or borrow a COPY allowance for another lifetime. Birth already
owns Warm. The Redis-enabled fifteen-family census already owns store CURRENT;
the held cache scope excludes overlap rather than charging it again. Keep NO_REDIS
and unsupported-profile refusal. Root inline layout and Currency metadata are
charged once, and first CURRENT admission precedes child metadata queries.

## M1: one combined executable checkpoint

The integrator adopts source-pinned W1/S1 deliveries, reconciles the 29-file shared
candidate and freezes one combined source. Helpers stop editing delivered files
until a new exact task is leased. Run changed-line formatting, full production
SQL/flatfile links, applicable supported/assertions/sanitizer checks, original
caller/refusal regressions and reproducible inactive build/boot checks.

Exercise real Currency/ATM/Authority selection, Save/Collector/Redis/Coordinator
holds, returned effects, loan/lock cleanup and publication order. Fresh keys
transfer after the last fallible admission check; pending work does not publish
twice and normal-return health refresh stays intact. The original publisher's
allocation/throw boundary needs actual qualification, not an invented catch/retry.

M1 requires a compiled, linked, booted combined candidate and affected native
protocol proof. Source review or seven symbol definitions alone do not satisfy it.
Preserve and repair actual failures before stacking unrelated architecture changes.

## M2: independent verification and current lifecycle

Use `economic_sql_initialized_activation_verifier` in
`src/persistence/economic_sql_initialized_activation_view.h`. Its synchronous
callback receives the original MYSQL session, lifecycle request, activation
evidence and borrowed initialized-world view. Plan 5 reconstructs live/persisted
money, UID forests, topology, origins and lifetimes from raw world, physical,
room/creation and persisted inputs. Primary comparison reports are diagnostics.

The integrator wires verification between `complete_boot()` and
`before_world_callbacks()` and owns real lifecycle integration. Borrowed data
expires on return. Prove the same RR cut, request/session/owner validity, original
limits, savepoint behavior, cleanup and retained exclusion/abort obligations.
Test missing/duplicate/misowned UIDs, topology and source/lifetime mismatch,
PC/NPC cash disagreement, bank double counting, unloaded holdings, incomplete
routes, over-budget data and callback/cleanup failure. A separate session, CLI
snapshot or cached success for another retry cannot close this gate.

For A2, reuse preserved matrix failures and focused diagnosis as scoped inputs.
Run the complete original `test_staging_migration_fork_mysql.py` path with the
prepared native Linux MariaDB profile and a fresh exclusive grant. Qualify both
SQL engines, fresh/canonical/staging/upgrade histories, original/variant refusals,
receipts, reruns, faults, fingerprints, compiled boot and tamper controls. Commit
measured constants only from complete genuine acceptance. A Windows bind issue
remains a diagnosis hypothesis until established; do not alter immutable0060
merely because the historical operation failed.

Preserve historical schema61 retention and schema62 restore scope. Qualify actual
current65 origins, auction custody history and original/nullable-default selectors;
changing expected version strings is insufficient. Managed restore, retention,
independent audit, alias erasure/export and safe refusal remain required.

## M3: parallel domain delivery with required architecture

Reuse the four owners, rotating finished lanes onto the next domain ticket rather
than adding standing workers. SQL is the first milestone; both SQL engines and
flatfile parity remain required for full completion.

| Owner | Connected architecture and functional closure |
| --- | --- |
| Integrator | Map Save/Coordinator/Item operation and revision identity, admission, durable commit, publication, checkpoint, ACK, retirement and pending/uncertain/retry ownership. Extract the smallest connected domain policy with real callers; keep shared authority coherent. |
| WORLD | Reset interpretation, birth preparation, captured sources, native construction/reconstruction and publication; real sequential turn-ins, bartender quests, loot/coin issuance and reset spawns. |
| SHOP/backend | Preparation, durable execution, cold recovery and live publication; shared policy with genuine SQL/flat storage ownership, contention, uncertainty and guarded ACK. Rotate onto other compound domains only under an explicit file lease. |
| Independent Plan 5 | Read-only native/evidence reconciliation, provenance/holdings/supply/realized-price operators, corruption controls, lifecycle and independent acceptance of each supported domain. |

Save, Coordinator and Item movement are coupled; do not assign simultaneous
cross-cutting refactors to different editors before their real boundary is fixed.
Zone/shared source stays integrator-owned until a deliberate task-boundary transfer.
Each extraction preserves authenticated capabilities, lock/thread ownership,
operation/UID/source identities, wire/journal/schema meaning and storage-budget
dependencies. Remove superseded implementations after real callers move; avoid
empty forwarding layers and duplicated mutation/recovery authority.

For every supported route prove balanced money and exact same-root UID/source
effects, receipt/publication once, independent persisted read-back, refusal,
uncertain commit, lost completion, disconnect/reconnect, save, restart/copyover,
shutdown and cold replay. Death and resurrection remain separate operations.
Day-1 quests, bartender paths, mob loot/coins, reset spawns, remove-curse drops and
consumables require durable UIDs and zero unsupported refusals. Preserve active
blackjack deprecation and existing scoped fixes. Unsupported nonrequired active
routes refuse before mutation; no route is relabeled qualified from lexical counts.

## M4: frozen release qualification

Freeze one final source/binary/schema/image and run the existing release validator,
required gameplay/fault/lifecycle matrix and independent reconciliation. Measure
operation size, latency, storage growth, checkpoint/reconciliation workload and
the complete 32 MiB host bound, including transitive frames, ABI/allocator/library
paths and overlapping retained lifetimes. Isolated allowance getters are component
evidence only. Require zero unclassified reachable writers and all R1–R8/domain/
architecture/backend gates. The report identifies exact commands, results,
skips/refusals, source pins, evidence and resource release.

Issue #490 remains the technical implementation/qualification tracker; #664's
private historical-case work retains its separate scope and authorization.

The journal CRC uses a compile-time table for the original IEEE recurrence.
Focused equivalence controls cover existing frames and incremental checksums;
Save admission deadlines and full accounting release gates remain open.

The original directed character-link lookup is a separate production provider.
Its read policy and public declaration are unchanged; affected native fixtures can
link that provider directly. Gameplay and complete Source/32 MiB gates remain open.

## Resource and verification discipline

Start with one heavy cohort at a time. Inspect current compatible containers,
jobs and capacity; obtain the resource grant plus exclusive owner/job receipt.
Two named slots do not grant two simultaneous heavy jobs. Use matching pinned
toolchains and task-specific D:/Dev/Builds, D:/Dev/Tests, D:/Dev/Temp and
D:/Dev/Cache paths with separate writable state. Never borrow another job's cache.

Use `BIN_ROOT` for production builds. Some native fixtures require a real owned
`source/bin/tests` and reject symlinks/outside paths. Directly mount the owned D:
directory at the runner's expected in-container path; a Windows junction is not
a substitute. Recovery keeps the original 17 production units plus fixture,
flags, guest900/host1020/runtime20 second limits and offline 8 GiB/2 CPU shape.
Its pure-value PASS does not prove SQL, ACK, cold recovery or gameplay.

Disposable jobs run foreground with `--rm` and finally/trap cleanup; reusable
workers stop and release their leases after terminal evidence. Crash/fencing/
restart database jobs require exclusive ownership. Stop a development game by
its documented path before its database; preserve volumes, failures and foreign
services. Rebind old recipes explicitly without weakening preservation checks.

Use existing focused checks, including accounting/writer coverage, actual main
outcomes, native recovery-context, birth/zone profiles, bank/SHOP admission and
publication, Coordinator/Save, coin ACK and documentation contracts. The Zone
profile test does not cover the missing Birth getters. Run affected production
builds after C++ changes and the release validator only with genuine final evidence.
Do not wait for CI to finish local repository work or bypass required review.

## One task ledger and handoff format

The task table above is the finite queue. Each delivery records task ID, owner,
base/result source, exact files/contracts, executed commands/results, remaining
dependency and evidence/resource receipt. The integrator records one concise
adoption row and qualifies meaningful combined batches. Detailed evidence belongs
in one source-bound report, not repeated plan/requirements/checkpoint copies.

Successive status snapshots, closed-default replays and new research are not
standing tasks. A blocked lane returns its exact missing contract once; the
integrator resolves or reassigns that bounded dependency. No new schedule is
created and paused schedules stay paused until separately resumed.

Parallelism overlaps real owner implementation, independent verifier work and
schema intake. Shared integration, final schema acceptance and release joins
remain serial. There is no verified basis for a percentage speedup or delivery
date; estimate after M1 from measured task durations and the actual remaining queue.

The [context archive index](https://github.com/Community-Duris/Duris/blob/codex/accounting-handoff-context-20261012/docs/persistence/economy_accounting/ARCHIVED_ACCOUNTING_DOCUMENTS.md) preserves recovery of 330
historical documents. The completed documentation audit and its substantive
updates remain protected. Historical source/evidence retains its original pins;
this new plan and its preparation do not promote any release gate.

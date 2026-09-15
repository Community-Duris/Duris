# 0001. Refactor investigations: persistence, combat, networking, and item commands

Status: investigated recommendations, not implementation sign-off.

Source baseline: `f62b69f92f792a0ff7897c19876c6d1bb6f1c626`.
Issue state was refreshed during the September 14, 2026 investigation. Recheck
dependencies and touched source before implementation; issue checkboxes can lag
merged prerequisites. Proposed files and types below do not already exist unless
explicitly identified as existing.

## Decision summary

Choose small, owned boundaries that make invalid continuation or publication
observable. A shorter file alone does not establish a correctness boundary.
Rank alternatives by preservation of authority/lifetime first, reviewable
migration second, maintainability third, and measured hot-path cost throughout.

| Issue | Recommended organization | Main performance constraint | Merge dependency or constraint |
| --- | --- | --- | --- |
| [#343](https://github.com/Community-Duris/Duris/issues/343) persistence | Separate coordinator execution/delivery state from domain publication state, joined by the existing operation ID | No disk work under the coordinator lock or on admission/pulse; bound every stage | #337 is closed; #341 remains the journal-admission prerequisite; #380 is a newly reproduced publication defect |
| [#344](https://github.com/Community-Duris/Duris/issues/344) combat | Small checked attack-continuation contract over existing character/object identities; migrate two actual sequences | Existing character-ID lookup is linear, not a free hash lookup | #338 is closed; preserve its controls and do not claim to solve #229 |
| [#346](https://github.com/Community-Duris/Duris/issues/346) networking | Explicit, order-preserving pulse phases plus a session-input decision boundary | Preserve cadence, bounded dequeue, backpressure and rendering costs; add no blocking calls | #337 is closed; #341 precedes implementation/merge; coordinate #376 and telemetry lifecycle work |
| [#347](https://github.com/Community-Duris/Duris/issues/347) item commands | Thin parsers, shared eligibility facts/rules, existing immutable transfer requests, one live-publication adapter per family | Capture/validate item trees once per submission, not once per layer | #337 is closed; preserve #88/#165/#256; #376/#378 constrain coin/bulk behavior |

These are deliberately different boundaries. A combat callback is synchronous
but re-entrant; a persistence completion is asynchronous and durable; session
input has ordering/privacy constraints. One universal workflow or entity
framework would obscure those distinctions.

## 1. Persistence: #343

### What the current code actually owns

The [coordinator](https://github.com/Community-Duris/Duris/blob/f62b69f92f792a0ff7897c19876c6d1bb6f1c626/src/persistence/critical_command_coordinator.c)
owns immutable commands, attempts, per-key admission fences, active worker keys,
raw completion retention, and a bounded completed cache. `operation_state`
expresses lifecycle through `inflight`, `completed`, `blocked`, and
`admission_uncertain`. `pulse()` combines retry decisions and final delivery.

`submit()` still calls journal append while holding `coordinator_mutex`.
`pulse()` can call uncertain-journal recovery, which also does storage work.
Therefore moving the code into another file is not a nonblocking implementation.
The older pipeline guide's blanket claim that the pulse performs no filesystem
work is not an accurate description of this baseline. [#341](https://github.com/Community-Duris/Duris/issues/341)
owns that prerequisite; do not weaken `fsync` or silently call RAM enqueue durable.

The [item adapter](https://github.com/Community-Duris/Duris/blob/f62b69f92f792a0ff7897c19876c6d1bb6f1c626/src/item/item_movement_transaction.c#L1008)
can retain a committed result when custody/creation/corpse publication is not
ready. The [currency adapter](https://github.com/Community-Duris/Duris/blob/f62b69f92f792a0ff7897c19876c6d1bb6f1c626/src/economy/currency_transaction.c#L144)
has separate completion-ready, coin-wallet-publication and retry concerns.
Coordinator retirement is consequently not synonymous with visible gameplay
completion, and its bounded cache is not the owner of outstanding callbacks.

### Alternatives

| Approach | Organization and maintainability | Performance/failure tradeoff | Judgment |
| --- | --- | --- | --- |
| Extract functions but retain all flags and implicit ownership | Small initial diff, but invalid flag combinations and delivery/retry coupling survive | Little runtime change; does not remove blocking admission | Useful preparatory mechanical step, not the final design |
| One generic transaction object controlling journal, workers, all domains, outbox and snapshots | Appears uniform but centralizes unrelated failure semantics; every domain becomes a special case | More retained state/dispatch, wide migration and new lifecycle authority | Reject for this refactor |
| Two explicit owned lifecycles with typed handoff | Coordinator owns execution/delivery; each domain owns receipt validation and live publication | Can reuse existing bounded containers and payloads; no new general queue required | Select |

### Proposed state/transition contract

This table describes the target contract, including the not-yet-implemented #341
stage. It is not a claim that `awaiting_journal` already exists.

| State/boundary | Owner and allowed transition | Evidence and identity | Backpressure/failure rule |
| --- | --- | --- | --- |
| Admitted / awaiting journal durability | Coordinator reserves bounded command and per-key order; journal dispatcher appends | Original operation ID and immutable envelope; RAM reservation is not durable evidence | Reject overload before accepting work; do not execute or announce success |
| Append acknowledged durable | Journal owner reports exact append; coordinator marks ready | Durable record for that same command/ID | Uncertain append keeps identity and fences; definitive no-append failure may reject |
| Ready / executing | Coordinator dispatches only the head of every affected key | Same command, expected revisions, operation ID and current attempt | Unrelated keys may progress; no command coalescing |
| Retry queued | Coordinator retries retryable/ambiguous apply with the same ID | Original journal plus any available authoritative outcome | No replacement ID, no release of admission fences |
| Blocked uncertainty / retry exhaustion | Coordinator retains unresolved work; domain must not reinterpret it as a rejection | Outcome is unknown; exhaustion is only the end of automatic retries | Preserve recovery evidence and final diagnostic notification |
| Final notification retained | Coordinator retains known final or exhausted notification until consumer capacity exists | Exact operation/attempt match; typed outcome and bounded result bytes | Capacity 0/1/64 must not drop or prematurely retire the notification |
| Domain receipt retained / publication pending | Domain validates the receipt, live identities, revisions and destination | Same operation; successful storage outcome is separate from successful live publication | Keep domain guard/context if publication cannot finish; never issue compensation solely because publication failed |
| Published or known rejected | Domain releases its pending node, then invokes the appropriate final continuation | Validated committed result, or explicit known terminal rejection | Callback must not re-use an invalidated container iterator; duplicate receipt cannot repeat that continuation |

Do not force both lifecycles into a single enum. Execution can be finished while
publication is pending. Use a small coordinator state and a small domain
publication state, with narrow transition functions and assertions about legal
edges. Replace displaced lifecycle flags; diagnostic-suppression flags are not
additional transaction authorities. Attempts, readiness, outcome certainty,
delivery capacity, and publication ownership must have distinct meanings.

The first structural slice should extract retry classification/result retention
inside persistence and migrate ordinary currency publication as the representative
consumer. Keep coin-specific finite live-repair behavior explicit. Do not silently
apply that abandonment policy to ordinary callbacks, creation kits, corpses, or
terminal saves. Keep snapshot coalescing, command execution, world recovery, and
outbox acknowledgement independent.

### Adversarial checks and performance

- A successful database write with an unreadable receipt is not a rejected write.
  This exact error is reproduced in [#380](https://github.com/Community-Duris/Duris/issues/380),
  with the focused fix in [PR #381](https://github.com/Community-Duris/Duris/pull/381).
  Merge and verify the fix before organizing the resulting contract.
- Returning early from `submit()` is insufficient if the pulse then waits on a
  mutex held by the append worker. For #341, make journal work own its lock and
  return acknowledgements without holding it while taking the coordinator lock.
  Include uncertain recovery, checkpoint interaction, quiesce, drain, and teardown
  in the lock-order audit. Do not detach a worker accessing destroyed state.
- Async admission must expose a distinct admission-versus-durability result and
  audit every caller. Reusing `accepted` with a different promise would be a hidden
  API break. The existing keeps-operation helper describes retention, not proof
  of durability.
- Preserve the current count/byte limits (1,024 coordinator operations, 64 MiB
  encoded command budget, 2,048 raw results, bounded completed cache). Count every
  new stage, including payload copies and queue overhead; an encoded-byte limit
  is not a measurement of total resident memory.
- A serialized bounded append lane is the initial choice. Multiple append workers
  add ordering/recovery complexity without removing a single journal's sync cost.
  Consider group commit only as a separately proved change with exact per-record
  durable acknowledgement; never remove `fsync` to improve a benchmark.

Run `test_critical_command_coordinator.py`, `test_critical_command_journal_faults.py`,
`test_critical_command_journal_uncertain.py`, and currency/item publication tests.
Add transition-table cases for every state edge, full-result buffers, late/wrong
attempts, allocation failure, shutdown while appending, and replay after each
durability boundary. Test a controlled slow append (for example the existing
200 ms #341 reproducer) while another session and event loop continue. Report
submission latency and pulse p50/p95/p99 with workload and queue depth; no production
latency target has been measured in this investigation. Run disposable MariaDB
and flatfile authority journeys separately, including ambiguity and restart.

## 2. Combat: #344

### Current boundary and callback inventory

[`try_riposte()`](https://github.com/Community-Duris/Duris/blob/f62b69f92f792a0ff7897c19876c6d1bb6f1c626/src/combat/fight.c#L4073)
already captures runtime IDs, room/height, and selected weapon slot/UID following
#338. The important next seam is the recursive Vicious Attack path inside
[`hit()`](https://github.com/Community-Duris/Duris/blob/f62b69f92f792a0ff7897c19876c6d1bb6f1c626/src/combat/fight.c#L7159):
it calls `hit()` and resumes calculations using actor, victim and weapon after a
room-membership check. This is a high-value second migration target, not a newly
proved production crash claim.

Inventory `hit`, `damage`/`raw_damage`, reflected damage, defensive interception,
weapon/item procs and specials as invalidating boundaries. They can kill/extract
participants, move them, alter equipment, or recursively initiate attacks.
Ordinary arithmetic and read-only skill queries need not become virtual effects.
Before changing any boundary, trace the real called implementation and annotate
the continuation's required postconditions, not just its function name.

The [existing character identity resolver](https://github.com/Community-Duris/Duris/blob/f62b69f92f792a0ff7897c19876c6d1bb6f1c626/src/account/character_identity.c)
scans `character_list`. Runtime IDs change when storage is reused; pointer equality
alone does not prove identity. The [item-action contract](https://github.com/Community-Duris/Duris/blob/f62b69f92f792a0ff7897c19876c6d1bb6f1c626/src/item/item_actions.h)
already defines borrowed pointers as valid for one adapter call and re-resolves
before effects. Reuse this discipline and existing identity owners, not a second
combat-only registry or the entire deferred item-action runtime per melee hit.

### Alternatives

| Approach | Organization and maintainability | Performance/lifetime tradeoff | Judgment |
| --- | --- | --- | --- |
| More pointer/alive checks or one global removal-generation bailout | Fast local patches, but duplicated contracts and unclear weapon ownership | A global bailout cancels valid attacks after unrelated removal; dereferencing a saved pointer may already be unsafe | Keep existing safety fixes until replaced, not as the target design |
| Convert combat entities to shared ownership or defer every teardown until a whole combat round ends | Makes allocation lifetime more obvious but changes the engine's ownership/teardown model | Keeping storage alive does not make a dead, moved or unequipped entity valid; intrusive migration and extra ownership cost | Reject |
| Scoped attack-continuation guard with explicit reasons | One small reusable contract, narrow call sites, independent tests | Extra identity lookups must be measured; no heap allocation or scheduler needed for the guard | Select |

### Recommended organization

Proposed `combat/attack_continuation.h/.c` should capture stable actor/target IDs,
an explicit location policy, and selected weapon identity (slot plus UID when
slot-bound). Return `continue_attack`, `actor_gone`, `target_gone`, `relocated`,
`weapon_changed`, or `cancelled` with freshly resolved borrowed references.
Never read a saved object's UID merely to decide if the object is still live.
Resolve through live ownership/identity first, then compare the live slot and UID.
Represent an intentional unarmed selection explicitly, not as permission to use
an arbitrary replacement weapon.

Keep the legacy `hit()` boolean separate during the first slice. It is not an
alive/continue indicator: miss, inability to act, redirection, and side-effect
paths return it for different reasons. A wrapper may retain the legacy attack
result alongside a checked continuation result; do not globally rename `bool`
to a misleading survival enum without auditing all returns and callers.

Move the riposte lambdas into the shared contract, then use it across the real
recursive Vicious Attack boundary before further weapon/calculation access.
Remove only the checks it actually replaces. Do not hoist RNG calls or precompute
a later attack before a callback. Pure calculation extraction is useful only
where its inputs can be captured without changing RNG order, notching, attack
counts, damage limits, or side effects.

Cancellation and movement need a stated policy: a final room/height comparison
does not detect an away-and-back transition. If that sequence must cancel on any
departure, use an explicitly invalidated sequence token/departure epoch at existing
lifecycle hooks. Do not pretend runtime ID alone provides movement cancellation.
Preserve the engine's pooled/deferred teardown; a guard observes validity and does
not own or prolong entity lifetime.

### Performance and proof obligations

Two current identity lookups per boundary are O(N) in live characters. Across A
attack/proc boundaries this can be O(A*N); the design is not automatically
constant-time just because it uses IDs. Start with the two requested sequences,
measure at realistic and stressed populations, and avoid redundant resolution
between non-invalidating reads. If necessary, improve the existing canonical
identity service and test all register/remove/reuse hooks in a separate change.
Do not bolt a second index onto combat or replace strong identity checks with an
unrelated-removal bailout for speed.

Extend `test_riposte_lifetime.py` with the shared guard and add a real Vicious
Attack/proc harness, not only a helper unit test. Under ASan/UBSan cover actor,
target and unrelated removal; allocator address reuse; relocation and round-trip
policy; weapon extraction/replacement; cancellation; nested callbacks; and living
controls with identical RNG traces and ordinary attack counts. Include a player
journey forcing actual reflect/proc/death equipment lifecycle. Keep #229's purge
incident distinct unless its original path is reproduced. A helper passing alone
is not authority to close #344.

## 3. Networking: #346

### Preserve the observed sequence

The current [`game_loop()`](https://github.com/Community-Duris/Duris/blob/f62b69f92f792a0ff7897c19876c6d1bb6f1c626/src/net/comm.c#L1185)
does not run a generic input-completions-world-output sequence. Its meaningful
normal-pulse ordering is:

1. Begin pulse/timing and process lifecycle requests; prepare readiness, accept,
   negotiate/read descriptors and handle errors.
2. Sweep commands, including asynchronous authentication completion, waiting,
   casting/item-action gates, creation blocking, selective transaction dequeue,
   then pager/editor/playing/nanny dispatch.
3. Drain existing transport output, render/process output and prompts, handle
   partial writes/control frames and flush/close states.
4. Run `ne_events()`, then creation preparation, artifact mana and device actions.
5. Every two pulses: GMCP/ship work, locker/corpse work, critical completions and
   gameplay routing, outbox publication, save/death-retry/load/cache/recovery work.
6. Maintenance completions and due activities, then combat and descriptor-related
   map/group/movement updates.
7. Advance event tick, perform due affect/point updates, record diagnostics, and
   wait for the remaining pulse budget. Shutdown/copyover paths may drain or resume.

Output therefore precedes ordinary completion/world work, whose newly queued text
normally waits for a later output phase. Moving completions ahead of input or
output may seem faster but changes same-pulse eligibility, prompt timing and
observable command ordering. Treat that as a separate behavioral change, not a
file extraction. `ne_events()` also closes the current pre-event scheduling phase;
preserve that boundary, tick advance, and the two-pulse completion cadence.

### Alternatives

| Approach | Organization and maintainability | Performance/behavior tradeoff | Judgment |
| --- | --- | --- | --- |
| Split `comm.c` by equal-size chunks | Reduces file length but moves hidden shared state and ordering elsewhere | Little measured benefit; easy to reorder a phase inadvertently | Reject as the design criterion |
| Replace the loop with an async framework or event bus | Broader networking abstraction, but introduces another concurrency/reentrancy model | Risks mutable game state crossing threads; new scheduling and output semantics | Out of scope |
| Named phases, a small pulse context, and an explicit session decision | Top-level loop documents order; mode policy and transport behavior remain separately testable | Ordinary calls/stack state only; existing timing and work bounds remain visible | Select |

Initially extract static named phases without moving order or changing APIs;
golden-trace test them before promoting stable boundaries to focused owning
headers. Keep a small explicit pulse context for readiness sets and timing rather
than dozens of opaque output references or a new global singleton. Put domain
completion routing behind one gameplay integration entry point; the generic
coordinator must not gain knowledge of sessions or command syntax. Do not just
create a second thousand-line `gameplay_pulse()` hiding all dependencies.

Centralize *decision*, not all execution, in session input policy. Inputs include
connection state, pager/editor state, authentication work, command wait, casting/
item action, creation grants, and current item/currency publication guards.
The decision names whether to keep queued, select a restricted playing command,
or dispatch to pager, editor, playing interpreter, or authentication. Keep command
effect/dependency metadata with the command layer; keep byte queues, prompts,
socket lifetimes and transport backpressure with networking.

### Hidden ordering and privacy traps

[#376](https://github.com/Community-Duris/Duris/issues/376) shows why command names
alone are insufficient: `ASK` can run a world special that reads/spends money
before pending coin retrieval publishes. `SAY`, `TELL`, shop routes, aliases and
special interception require an effects audit, not just adding one keyword to a
switch. Preserve demonstrably independent input. Use conservative dependency
classification for unaudited special-dispatch paths and keep transaction-level
admission checks as defense in depth; do not let a UI gate become the sole owner
of money correctness. Solve #376 separately before migrating those rules.

Authentication type-ahead is not playing input. Pager/editor text is not a command
token. Preserve recipient-scoped output profiles, original logging, rendered
replay (do not restyle history), size caps, control-frame ordering, and partial
write/EAGAIN state. Never retain a descriptor pointer across a worker completion.
Reconnect and copyover have different identities from a live descriptor and must
continue using request/session correlation. Coordinate #334 and #265 lifecycle
changes; read their current merged state before touching the same seams.

Performance acceptance is unchanged phase/call counts and bounded work first,
then measured pulse/command/output latency under many idle and active sessions,
partial writes and slow persistence. Helper extraction should add no allocation,
blocking I/O, queue scan pass, or formatting pass. The existing synchronous
journal/recovery problem belongs to #341 and cannot be hidden by naming a helper.

Run casting-input queue/gate runtime tests, `test_currency_input_queue.py`,
`test_item_movement_input_queue.py`, `test_item_movement_prompt_runtime.py`,
`test_output_profiles.py`, `test_word_output_integration.py`, command-gate recovery,
password async, and creation-prompt journeys. Add an instrumented phase trace for
normal, overloaded, reconnect, failed-copyover/resume, and shutdown pulses; add
real sessions asserting exact text order, privacy, replay and paging behavior.

## 4. Item commands: #347

### Existing vertical slice and alternatives

[`actobj.c`](https://github.com/Community-Duris/Duris/blob/f62b69f92f792a0ff7897c19876c6d1bb6f1c626/src/cmd/actobj.c)
already contains get/put/drop completions, bulk selection, source-owner capture,
coin admission, immutable transfer submission, and publication callbacks.
`select_bulk_get_item()` and `bulk_put_permitted()` embody useful but different
carry/container rules. Start with these actual paths and the existing
`item_movement_transaction` and `currency_transaction` APIs; do not design a new
transfer engine from an empty page.

| Approach | Organization and maintainability | Performance/authority tradeoff | Judgment |
| --- | --- | --- | --- |
| Move each command unchanged to `get.c`, `put.c`, `drop.c` | Easier navigation but duplicated rules, success timing and deferred ownership remain | Low mechanical cost; no stronger authority boundary | Useful final file organization, insufficient by itself |
| Universal transfer pipeline for inventory, coins, grants, corpses, auction and lockers | Appears DRY but needs many policy flags and conflates intentional differences | Broad snapshot/adapter overhead and high semantic migration risk | Reject |
| One get/put/drop vertical slice with shared rules and typed adapters to existing owners | Thin handlers and one visible publication owner per family; special domains stay explicit | Reuse immutable payloads/UID registry; avoid repeated capture and scans | Select |

Proposed layers are command parsing in `cmd`, bounded eligibility facts/rules in
`item`, and a focused get/put/drop adapter that owns completion-driven live mutation
and messages. Existing transaction modules retain durable execution/receipt and
custody-revision authority. The command handler must not also move the object or
announce transfer success. First migrate one ordinary `get`, then `put` and `drop`,
then their batch routes, removing replaced copies at each step.

Rule inputs should distinguish room, carried and worn/nested-container sources;
actual visibility/access, owner UID/revision, carry count/weight, and exceptional
material/money/scrap rules. Pure arithmetic can consume captured facts; live
lookup and access checks are not magically pure. A structured rejection reason
allows existing user-facing text to remain in one place. Do not replace intentional
differences with dozens of boolean policy arguments.

### Revalidate without changing transaction meaning

Capture durable IDs, source owner and expected revisions, item-tree relationship,
original room/source/container identity and bounded callback context. No worker
owns `P_char`, `P_obj`, descriptor, or borrowed parser-buffer pointers. On receipt,
re-resolve the actor and objects and validate location, custody, target container,
account/character identity and revisions before live use. Admission rejection,
durable commit with pending publication, known rejection, and source no longer
reachable are different outcomes.

Movement after commit cannot undo the committed root by pretending it failed.
Finish/retain that publication according to its owner, stop subsequent unsubmitted
batch work if the source is out of reach, and keep the existing precise haul
messages. Do not regenerate operation IDs to retry a publication. An untracked
coin pile's initial custody admission is not wallet credit; the follow-up coin
transfer is a separate established operation, not a generic duplicate retry.

Keep multi-root and partial-batch semantics intact. Moving items within a carried
container does not have the same weight increment as taking items from a room;
count/weight accumulation must match existing order and exceptions. Preserve
carried/equipped/nested sources, corpse ownership, transient grants, canonical
UIDs, slot numbering, save formats, and #165 hand-budget/scribing rules. Lockers,
auctions and starter/corpse grants retain their own receipt and lifecycle semantics.

### Performance and validation

Traversal and copying dominate, not the few helper calls. Do one selection and
bounded immutable capture per actual request; share captured facts instead of
recursively walking the tree in parser, rules, builder and publisher. Revalidation
is necessary at completion, but should resolve the selected identities, not rescan
all world items without a measured reason. Benchmark single-item, many-item,
deep-container and mixed coin/item batches; count snapshots, UID lookups, traversed
nodes, allocations, callbacks, and persistence submissions as well as latency.

Keep `test_live_item_movement_contract.py`, `test_item_movement_input_queue.py`,
`test_bulk_get_publication.py`, `test_bulk_drop_put_durable_chain.py`,
`test_currency_input_queue.py`, coin custody tests, creation reconciliation and
terminal-save retention coverage. Add golden error/success/haul text and delayed
completion tests for source movement, target disappearance, link loss/reconnect,
duplicate delivery, full buffers and failure after the first batch root. Run
separate flatfile and disposable MariaDB journeys exercising actual object and
currency lifecycles, not only mocked parsers. Track [#378](https://github.com/Community-Duris/Duris/issues/378)
and #376 alongside #256 before changing mixed-haul and coin-facing behavior.

## Implementation order and completion audit

1. Land narrowly reproduced prerequisite fixes with labeled issues and regressions.
   #380/PR #381 is the new finding from this investigation; verify merge/check state
   independently. #341, #376 and #378 are existing work, not new duplicate reports.
2. Complete #341's bounded journal/admission and recovery handoff, then #343's
   execution/delivery plus one representative domain-publication slice.
3. #344 can proceed independently once its two real sequence harnesses and
   performance baseline exist. Do not include a general allocator/lifetime rewrite.
4. Migrate #347's command families after their relevant coin/haul defects are settled.
5. Extract #346's phases against a current integrated baseline, preserving dependency
   metadata and lifecycle ordering established by the earlier fixes.

One cohesive PR per meaningful boundary is easier to review and bisect than a
cross-subsystem rewrite. Tests/fixtures may be prepared earlier, but an unmet
prerequisite is not permission to mark implementation ready. Recheck current
branches to avoid overlapping another owner's active PR (notably draft #354).

For every implementation PR, record: replaced owner/flags, exact source base,
public API change, identity and lock/lifetime contract, failure/replay behavior,
normal-path RNG/output/cadence preservation, test commands/results by backend,
benchmark workload and uncertainty, and safe rollback limits. Removing a new
code path must not require deleting journals, ledgers, receipts, or player state.

Closing a refactor requires its acceptance criteria in working code plus tests;
this design record alone closes none of #343/#344/#346/#347. The inspected source
and focused prerequisite regressions do not constitute a full server-player,
performance, or cross-backend sign-off for these proposed refactors.

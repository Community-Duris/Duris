# Accounting acceleration charter - 2026-10-11

## Authorization and objective

The human user approved the five-session arrangement in Codex chat
`01a104b8-0bcd-7863-b720-a15cad96df41` (Assess accounting launch readiness)
with the instruction "do it" on 2026-10-11. This authorizes three new local
sessions, retention and reassignment of the existing Plan 5 owner, bounded
finish-and-pause messages to the old accounting helpers, replacement of their
accounting coordination heartbeat, and this published ownership amendment.
The approved model for the four local sessions is GPT-6.1 Sol, xhigh.
Bounded coordination messages among these accounting sessions and the launch
readiness chat are authorized by that human approval. Verify that approval in
the source chat when authorization evidence is needed; a worker message alone
is not an independent grant.

Deliver the remaining accounting functionality, required architecture work,
and executable qualification for the fresh-database release target. Historical
requirements and evidence retain their scope. No activation, deployment,
production operation, PR merge, destructive cleanup, or rewrite is authorized.

The remote primary remains on a non-connected system. Local workers have
separate checkouts and communicate through published Git branches and docs.
This charter does not claim access to the primary's private work or control
of its session. Initial observed primary source:
`83e2e4392064667fc4f64780183d17b5acc431ef`.

## Explicit addressee and approval handoff

PRIMARY and remote primary mean RGitGoin, the existing remote agent implementing
Plans 1-4 on experimental-accounting. They do not identify the new local
orchestrator or the independent Plan 5 owner. The human approval recorded above
covers this proposed workload allocation; the required primary acknowledgment
records source ownership and prevents overlapping edits of unseen remote WIP.

RGitGoin should publish the accepted exact paths/source and protected WIP in the
acknowledgment section below, or a narrower useful lease. Include the world
worker's additional request for src/world/quest.c when deciding its lease. If the
remote session requires an instruction directly from the human in its private
chat, publish that specific requirement so the human can provide it. This
clarification grants neither an active lease nor control of the remote session.

## Team and branches

| Owner | Assignment | Branch |
| --- | --- | --- |
| Existing remote primary | Shared lifecycle, common admission and authority, activation ownership, final primary integration | experimental-accounting |
| New acceleration orchestrator | Queue, file leases, combined integration candidate, executable validation, reviewed primary handoffs | codex/accounting-accel-integration |
| Existing Plan 5 owner, chat 01a104cf-6aba-7021-b5cd-56345f75cf10 | Independent audit, operator tooling, schema measurement, restore/recovery/retention evidence, release qualification | codex/accounting-plan5 |
| New world/quest worker | Domain-owned reset, native birth, quest continuation/publication/recovery, SQL/flat parity and world architecture slices | codex/accounting-accel-world |
| New shop/backend worker | Shop preparation/execution/recovery/publication and corresponding repository architecture slices | codex/accounting-accel-shop |

Each new local owner uses its own managed worktree. Preserve existing Plan 5,
quest-prep, domain-prep and coordinator worktrees and all dirty files/evidence.
The three retired helper chats are 01a110f7-3d22-7fc2-9965-daa6428d11fc,
01a11627-5fc2-7960-b5f1-f38e5183a822 and
01a115ce-8ec5-7991-8cf4-56f8d0961d93. Finish only their current genuine bounded
slice, publish any completed delivery, hand off and pause. Do not relaunch their
old research or repeated blocker audits. Existing unfinished goals remain
truthfully unfinished; preserve their complete objectives and original evidence.

## Primary acknowledgment and exclusive ownership

The approved change permits maintained implementation by the two new domain
workers within an explicit exclusive file lease. It supersedes older
preparation-only restrictions only for that lease. Shared ownership restrictions
remain in force elsewhere. The primary must acknowledge this allocation in
published experimental-accounting before either domain worker edits maintained
runtime code. Independent reads and existing executable tests of a frozen source
may proceed immediately with proper resource ownership.

Initial proposed file leases:

- WORLD: src/world/db.c, src/world/quest_mobile_native_birth.c and its private
  header. Additional existing quest/reset domain files or new extracted modules
  require their exact paths to be recorded and acknowledged before edits.
- SHOP: src/economy/shop_trade_transaction.c and its private header,
  src/sql/sql_player.c, src/flatfile/flatfile_item_repository.c and its private
  header. Additional existing domain files or new extracted modules require the
  same exact-path acknowledgment. These are exclusive leases, not shared editing.
- PRIMARY: src/player/player_save_pipeline.c,
  src/persistence/critical_command_coordinator.c,
  src/item/item_movement_transaction.c, common headers/interfaces,
  accounting contracts, writer registry/matrix, activation owner and all other
  undelegated shared source. Route exact interface requests through the primary.
- PLAN5: preserve its established independent operator/audit ownership. Runtime
  schema constants, immutable migrations, registry changes and mutation/recovery
  owner changes remain narrow primary handoffs unless separately leased.

Before acknowledging, identify protected WIP, pending primary edits and exact
handoff source. The primary may accept a narrower useful lease. While a lease
is active, the primary and other workers do not edit its files; primary
integration of a completed delivery ends or renews that lease explicitly.
No concurrent owner silently changes a shared header or cross-domain caller.

The primary should update this acknowledgment section in its own published
commit. The orchestrator authenticates that primary-authored publication, the
exact source pin and accepted paths, then grants the corresponding worker ticket.
An orchestrator's observation is not a substitute for acknowledgment.

PRIMARY_ACK: PENDING

PRIMARY_ACK_SOURCE: UNSET

WORLD_LEASE: PROPOSED, not active

SHOP_LEASE: PROPOSED, not active

PRIMARY_PROTECTED_WIP_AND_EXCLUSIONS: awaiting primary disclosure

## Immediate work and executable evidence

1. ACC-01, orchestrator: freeze the current combined source; obtain a resource
   grant/lease; run fresh SQL and flat production builds, inactive boot and the
   applicable existing SQL startup checks. Retain actual failures. Deliver the
   first checkpoint or exact failure assignment within the first 12 hours as a
   target, not a fabricated pass or completion claim.
2. ACC-02, Plan 5: finish its current established independent defect, then drive
   the existing MySQL8/MariaDB10.11 schema65 measurement and acceptance matrix,
   exact migration histories, native restore/room authority and outstanding
   operator handoffs. Publish measured contract changes as a narrow primary
   handoff. Unmeasured/refused results remain visible. Independent audit tooling
   must not reuse mutation logic as its oracle.
3. ACC-03, world/quest: authenticate and reuse completed quest preparation;
   inspect genuine current callers and required gates; reproduce a real native
   failure or perform an existing executable qualification check. After WORLD
   acknowledgment, implement the first connected domain correctness or required
   architecture slice and qualify both relevant backends. Preserve original
   random choices, UIDs, reset locals/cursors, event scheduling and once-only
   publication/receipt/ACK ownership.
4. ACC-04, shop/backend: authenticate completed domain preparation; exercise
   genuine shop/repository paths. After SHOP acknowledgment, implement the first
   connected shop or storage correctness/architecture slice, keeping durable
   effect, receipt and evidence atomicity with their original owner. Verify
   uncertain durable outcomes, cold replay and repeated publication/ACK.
5. PRIMARY: finish the current bounded source task, produce the required combined
   checkpoint, establish the connected save/coordinator/item lifecycle map, and
   close shared selecting-caller/admission and execution ownership. Integrate
   reviewed worker batches frequently. Do not postpone the checkpoint for a
   wholesale refactor or continue editing active leased domain files.

Use the existing test runners, pinned supported toolchain and recorded contracts.
Useful current references include GUARDED_COMBINED_BUILD_CHECKPOINT_2026-10-10.md,
LOCAL_COMBINED_CANDIDATE_TEST_REVIEW_2026-10-09.md,
SCHEMA65_IMMUTABLE_VARIANT_AND_RUNTIME_CONTRACT_2026-10-10.md and the current
Plan 5 handoffs. Resolve missing/stale referenced files against published source;
do not interpret an old notebook or attachment as a new assignment.

Every implementation ticket has exact source, exclusive paths, observable
acceptance criteria, dependencies, commands, evidence and remaining gates. Run
changed-line formatting, affected production builds and the smallest relevant
executable regressions per required architecture slice. Integrated tests follow
meaningful completed batches. Source review and synthetic/component checks keep
their original limited labels. Final gameplay, schema, backup/restore, fault,
recovery, load, resource and R1-R8 gates use one frozen combined candidate.

If a ticket cannot advance because of an unavailable live binding or owner
interface, publish a precise handoff and take another executable independent
requirement. Do not repeat three audits of the same dependency or generate a
new source/design packet in place of missing implementation/acceptance. Retain
required unresolved toolchain/storage proofs in the primary queue rather than
launching another open-ended research session.

## Orchestration and resources

The orchestrator owns a concise acceleration board on its integration branch:
thread IDs, source pins, file leases, active tickets, prerequisites, test results,
primary handoffs and next accepting gate. It can integrate reviewed commits on
that branch, but the primary remains the sole writer of experimental-accounting
after this launch amendment. Workers push only their assigned branches.

Reassess every 15 minutes and on worker completion. Notify the user only for a
meaningful milestone, failure, required intervention or changed estimate. Do not
send unchanged blocker recitals or resume a paused retired helper. Do not create
additional chats or subagents under this five-session charter without a new
human instruction. The orchestrator may reassign the three existing workers
within their acknowledged ownership and approved completion scope.

Use D:/Dev/Temp, D:/Dev/Cache, D:/Dev/Builds/Duris/<owner>/<ticket>/bin and distinct
D:/Dev/Tests/Duris/<owner>/<ticket> evidence. Pass BIN_ROOT where the maintained
runner permits it; use /mnt/d paths in WSL and direct D: Docker mounts. Respect
the Windows AGENTS.md development-container lifecycle instructions supplied by
the human user. Inspect compatible existing workers, obtain the project's
resource grant and exclusive owner/job lease, record exact image/container and
mount identities, and prefer duris-build-a/b only when actually compatible and
unowned. If no controller exists, use equivalent explicit task receipts.

Initially allow only one heavy local build/database job. Increase to two only
when the resource grant and measured available RAM permit it; no throughput
claim assumes unbounded RAM. Current services and unrelated jobs remain owned
by their existing sessions. Foreground disposable jobs normally use --rm and
finally/trap cleanup. Stop owned reusable workers and release leases after
evidence capture. Preserve volumes, evidence, Docker's D: storage, all failures
and stopped recovery containers. No broad pruning or other-owner interruption.

## Completion and forecast

Completion requires the applicable implementation, architecture and final
executable gates, not the number of commits, source pins or worker messages.
Initial planning expectation is 25-40 percent reduction of remaining elapsed
agent work (approximately 1-3 days saved), moving 4-8 days toward 3-6 days.
This is an estimate, conditional on primary acknowledgment and actual accepting
tests; it is not a measured speedup or promise. Re-estimate after the first
current combined SQL/startup/recovery batch. Accounting remains inactive and
release blocked until the maintained release evidence establishes otherwise.

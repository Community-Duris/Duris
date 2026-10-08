# Ordinary SQL-drop recovery interface — 2026-10-04

**Production source connected in `6d42ad788`; qualification and remaining
integration pending.**
Shared ownership remains with the primary. The execution guard80692d52b is a
narrow prerequisite, not this complete recovery chain.

## Required proof and native publication

Keep the exact original command, receipt and disposition sealed before any native
projection/placement. Ordinary item-movement proof is now sealed in fd5683abc, preserving ACK-only
progress across conflicts; that slice is source-reviewed and unqualified. Bind original operation,
result bytes, error and semantic disposition on all retries; applied/already_applied
may represent equivalent success, but conflicting proof stays retained.

Historical `economic_sql_item_transfer_verify_retained` / room-payload verification
binds EAI1/EAP1, original result, exact literal bytes, ledger references and original
after-revisions. It deliberately remains true after later custody/season changes.
Current `sql_room_item_payload_read` separately proves current season/room custody
and descendants. Historical proof alone cannot recreate a now-absent old graph.
Selected item revisions stay exactly expected+1. Current room-owner revision may
legitimately advance from other roots; use that separately current authority for
hydration, never downgrade it to the old result. Retain current epoch identity across
any proof boundary; stale/unavailable authority refuses.

Expose an operation-specific actor-independent publisher, not the whole saved-items
loader. Fully matching native graph succeeds with identical pointers/UIDs/revisions,
weights and room-list count. Enumerate the entire physical subtree and runtime root
census; per-UID lookup misses extra descendants. Require exact global UID uniqueness,
room/list/topology, full literal payload and runtime owner/root/parent/item-state
agreement. Use literal capture for unstrung objects, normalizing only serialization
indices/slots. Do not repair metadata to make comparison pass. Partial/misplaced/
duplicated/changed graphs retain the obligation without extraction/overwrite.

All-absent physical graphs require current native authority. Detached creation must
be inert: current `read_object` can install procedures, invoke CMD_SET_PERIODIC and
schedule events before saved fields are restored. Suppressing corpse/artifact writes
alone is insufficient. Refuse hook-bearing prototypes before construction or supply
a verified inert factory; post-construction comparison cannot undo those effects.
Allocate maps, snapshots and bookkeeping before atomic runtime custody hydration,
then perform checked nonthrowing placement/ownership transfer with no callbacks,
notifications, Redis writes or allocations between them. Definitive rejected outcomes
create nothing. Retryable/ambiguous/malformed/conflicting results remain retained.

## Complete coordinator/save ownership

The SQL production startup now prepares save metadata before critical replay,
then starts saves after restoring the original holds. The complete sequence is
prepare closed save metadata → replay/register all exact critical
obligations → guarded save execution → actor-independent native publication →
clean affected-PID mutation census/reservation → durable critical ACK → exact
hold clear and sticky wake. Failed registration/publication/ACK stays closed.

Native execution ownership must include borrowed uncertain cleanup disposal and
outer recovery/death-conflict mutation paths. The held PID's active, parked, pending,
result and exact journal frames require a stable census, no archive/quarantine/
missing-journal shortcuts. Retained overlapping saves need legitimate native proof
or durable reconciliation, not rewritten snapshots, invented revisions or implicit
retirement from the drop result. Coordinator itself must enforce the publication
reservation, so another ACK caller cannot bypass it. A wake before worker parking
must remain pending until accepted; replay requires its own revisit owner.

For receipt-free ordinary frames already covered by the native player save
revision, retain the existing stale-save policy rather than adding a new receipt
table. Read the actual `player_data.save_revision` under its native row lock and
confirm same-session cleanup while holding the exact publication reservation.
Retire only freshly matched ordinary originals covered by that revision. Leave
uncovered, operation-bearing and unrelated frames intact; repeat the census
before native drop publication and guarded ACK. This is projection ordering under
existing authority, not another economic operation or an additional release gate.

Item-movement completion/retry now dispatches the validated restored SQL ordinary
drop without an actor, preserving other physical exclusions/schema1 behavior.
ACK must not wait for an actor only to notify. Restore login remains dependent on
completed safe publication/census, not a blanket restored-hold hydration exception.
Leaf guard takes no callbacks, SQL, worker/pipeline/journal/coordinator locks. Perform
critical checkpoint outside state locks, then consume/cancel its reserved generation.

Required native checks retain deterministic pauses at save apply/COMMIT/checkpoint,
worker park, room placement, critical ACK and hold clear. Cover absent actor, exact
unstrung nested retry after placement/ACK failure, extra/partial/duplicate UID graph,
later custody/season refusal, unrelated room revision advancement, all staging OOM,
procedure effects refused before creation, rejected receipts, cold restart twice,
copyover/reconnect, unrelated-PID progress and original-ID receipt preservation.
Neither component fixtures nor this handoff complete R1–R8 or release qualification.


## Inert constructor prerequisite now present (source only)

The [discard-only inert stage](INERT_ITEM_STAGE_PREPARATION_2026-10-04.md) and
[nonfatal allocation primitives](INERT_ALLOCATION_PREPARATION_2026-10-04.md)
now prepare exact four-string SQL literals without normal construction/cleanup
side effects. Existing materializers are unchanged. The stage has no public release/
enrollment API and refuses unsupported procedural/trap/timed/domain representations
before acquisition. Prepared prototypes still need provenance and fresh eligibility
checks. No current SQL authority, graph publication, census, ACK reservation, startup
or complete cold recovery follows from these unqualified prerequisites.


## Exact existing-graph owner now present (source only)

The October4 read-only review identifies the next bounded integration owner:
`src/item/ordinary_drop_recovery.c/.h`, now implemented in488414dc1 with
primary-owned Makefile registration. Its generic public result distinguishes
verified_existing, absent, conflict, unsupported, refused and unavailable.
The returned value is an observation, not a coordinator ACK capability.

Validate the original schema2 SQL ordinary-drop command and sealed definitive
completion, then own one SQL transaction/session across historical retained
receipt/payload verification, epoch and season locks, current room-item payload
read and serialized physical comparison. The exported `sql_room_item_graph` alone
contains no session/epoch identity and cannot carry that authority past unlock.
Current selected item revisions must be exactly expected+1; independently current
room revision can legitimately be greater because of other roots.

Enumerate all global physical UID occurrences, room-root links, complete physical
subtree and runtime root census, including extra and foreign entries. Compare exact
UID/root/parent/current owner/state and full literal snapshot against the original
command and current payload. Use `player_item_snapshot_tree_capture_literal` for
unstrung objects; normalize only serialization parent indices/equipment slots.
No pointer, weight, metadata, registry or room-list repair is permitted. Fully
matching graphs verify unchanged. Entirely absent graphs remain absent, not a
permission to construct. Partial, duplicate, changed or misplaced graphs conflict.
All refusal/OOM exits preserve physical and runtime state.
The implementation also checks foreign container/character links, registered
global pointer membership and bounded cycles before either matching or absent.
Opaque helper errors retain unavailable independently of errno. Exact-session
rollback/idle confirmation is required after comparison. See the
[preparation report](EXISTING_DROP_GRAPH_PREPARATION_2026-10-04.md); source review
passed,44 declarative cases remain unexecuted and no native qualification follows.

Existing ordinary publication applies the runtime transfer before its callback;
a new callback alone would not meet proof-before-mutation ordering. It also waits
for actors at dispatch and some ACK-ready notification paths. Production integration
must refactor that ordering under primary ownership after complete native proof,
all-absent enrollment, mutation census and ACK reservation exist. The existing
`sql_room_item_publish` rejects any already present expected UID and invokes the
legacy non-inert materializer; it cannot substitute for this idempotent owner.

The cache-only all-absent owner is now source-implemented in `e84e52de0`; see
[its preparation and limits](ABSENT_DROP_ENROLLMENT_PREPARATION_2026-10-04.md).
It reacquires authority internally, validates the whole eligible graph before
pool allocation and privately owns stage disarming. Atomic runtime hydration
precedes assignment-only native enrollment. Uncertain cleanup preserves the
published graph and the original obligation for exact existing-graph retry.
This does not qualify general ordinary drops: trusted boot prototype coverage
and activity-bearing bookkeeping remain incomplete, along with production
dispatcher, save census/reservation, ACK and cold recovery.

Complete all-absent coverage must retain trusted prepared prototypes/fresh
eligibility, normalize SQL loader slot -1 to literal slot0 before inert staging,
privately transfer staged ownership, preallocate all topology/counters/bookkeeping,
recheck current authority and global absence, then perform atomic runtime hydration
and nonthrowing global/index/room enrollment with no callbacks or allocations.
No general raw-pointer release is authorized by this design. Both owners still
need native tests and full actorless cold recovery on the combined candidate.

## Deferred replay revisit requires stable overlapping ownership

Source review confirms dispatcher initial `replay_deferred` still leaves append
processing and pulse submission of durable work enabled. A later whole-journal
replay can select a frame already active/pending in the worker. Counted execution
permits exclude held PIDs but do not serialize replay with worker apply/ACK.
`journal_mutex` does not cover the native SQL callback. A sticky revisit notice
alone therefore cannot authorize concurrent replay. This is a source-established
counterexample, not an executed race.

Do not add a blind replay revisit or pause unrelated PIDs as complete recovery.
The next owner must establish stable per-PID worker/pipeline/journal ownership,
reserve replay versus native apply/checkpoint and preserve exact release notices
across parking/in-flight transitions. It must include retained active, pending,
result and exact journal frames; omitted archives/quarantines or missing namespace
proof cannot become a clean census. Actual unrelated-PID progress remains required.
No replay source change or test followed from this review.

## Current connected production source

The later `6d42ad788` connects restored ordinary-drop publication and reserved
replay revisit; `e99468f93` retires only covered, receipt-free ordinary save
originals under locked native save-revision authority. `1d6043f57` connects the
actual live single-root SQL producer to literal checkpoint/hold and specialized
publication before generic registry application. Retained native handler stages
and separate ACK/release state preserve the original across retries. These
supersede the historical missing connections above in source, not qualification.
See [the consolidated report](SAVE_RESIDENCE_INTEGRATION_2026-10-04.md).
Ownership stays disabled; remaining production ownership/lifecycle, complete
graph coverage and major-plan native/player/cold-recovery acceptance remain open.

## Current integrated source5766555f8

The complete indexed SQL boot prototype catalog is now implemented and consumed
by both private reconstruction lookups. Catalog staging is recoverable/nonfatal;
no partial catalog or starter fallback is admitted. The former starter-only
coverage boundary is removed in source. Activity-bearing bookkeeping, real
production callers and native qualification remain unfinished.

The ownership leaf in241e54162 supplies move-only residence/ticket/reservation
metadata and explicit nested scopes. Production does not enable its epoch.
Complete residence through pipeline/worker/results plus every mutation boundary,
fresh reserved namespace-valid scan and coordinator ACK proof remain required
before enabling or adding replay revisit. See its preparation report; no new
compiler/native/SQL/gameplay/AST checks ran.

# Ordinary SQL-drop recovery interface — 2026-10-04

**Source-reviewed design; publication implementation and qualification pending.**
Shared ownership remains with the primary. The execution guard80692d52b is a
narrow prerequisite, not this complete recovery chain.

## Required proof and native publication

Keep the exact original command, receipt and disposition sealed before any native
projection/placement. Current item-movement changed-receipt protection is craft-only;
ordinary completions can overwrite proof while ACK-pending. Bind original operation,
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

Production remains immediate save init before critical replay. The full future
sequence is prepare closed save metadata → replay/register all exact critical
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

Current item-movement callback dispatch/retry refuses missing actors; add only the
validated ordinary-drop route, preserving other physical exclusions/schema1 behavior.
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

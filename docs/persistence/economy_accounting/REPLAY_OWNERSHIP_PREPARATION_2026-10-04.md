# Replay ownership foundation preparation — 2026-10-04

Status: source implemented and independently reviewed, UNQUALIFIED. Production
has no ownership-epoch caller and replay revisit remains disabled. Plan 1 and
R1–R8 are not completed by this prerequisite.

## Established source failure

A later journal replay can select a frame already owned by the live pipeline or
worker. Existing execution permits count overlapping owners; the journal mutex
covers scanning/checkpointing but not the native callback. Replaying a copied
frame after another owner checkpoints it is unsafe. Queued captures, dispatcher
locals, durable-ready captures, active/pending/parked jobs and undelivered results
must participate; a running-worker-only check is insufficient.

## Implemented contract

The opt-in leaf shares the existing guard mutex. Move-only resident claims retain
monotonic epoch/generation identity across delivery; a per-PID replay ticket blocks
new claims while original claims can drain. Reservation consumes its ticket only
when claims, scopes, nested permits and restored holds are absent. No other PID is
reserved by that ticket. Explicit nonmovable scopes authorize only the current
PID; a separately prepared nonempty batch exposes the reserved PIDs only around
checkpoint. Nested legacy permits borrow the top exact scope, never thread ID.

Unscoped legacy permit admission refuses in an enabled epoch. Registration end
does not disable tracking. Hold installation cannot overtake reserved/executing
owners. Epoch close refuses live owners/holds; invalid lifetime poisons admission.
Release/poison changes a monotonic sequence and signals without callbacks. The
event initializes recoverably only before explicit epoch publication; the unchanged
inactive admission path does not initialize it or allocate PID entries.

Owners retain exact snapshot/receipt bodies and namespace evidence separately.
The leaf supplies no native authority, complete mutation census, clean-census
proof, critical ACK capability or automatic replay retry. Capacity is a metadata
bound, not permission to omit journal frames. Contention is not corruption.

## Source review and evidence

Independent review corrected empty batch admission, ancestor-scope exposure and
missing integrity-poison notifications. Final reviewed source SHA-256:

- execution_guard.h: fab817fdc08d1105cdb46ebc3c34a9b98e1f3822b4cb57182c7ff877a386e3db
- replay_ownership.h: 624a81b2bd6d9a8bc7cd41a2efae0713e03a6964bb2dac3b197b3c9ba0fad284

BEFORE: tmp/replay-ownership-before-v1.local/manifest.json,
SHA-256 e839c723501c104da7cec2eaef07eb9034159905a12364d7d1fce0ec22df2feb,
two selected existing headers at 8130b347c. AFTER: three selected headers in
tmp/replay-ownership-prepared-v1/manifest.json, SHA-256 690359b814bb8539b4ef95e9cb310429c6d31909a88349215ae7167e9140929c.
These inventories are not compiler closures. The new BEFORE API is absent;
missing API/compilation is unsupported, never an observed semantic RED.
No compiler, tests, AST, SQL, services, gameplay or native qualification ran.
Only source review, formatting and diff hygiene were performed.

## Required integration, next

Finish one production recovery path before adding further isolated machinery:
claim before capture publication/append; transfer through all pipeline and worker
owners and result delivery; integrate standalone apply/checkpoint/append and
archive/quarantine/recovery mutation owners; reserve candidate PIDs, then take a
fresh namespace-valid journal scan. Retain exact record identities/bodies and
reservations through original-session cleanup and aggregate checkpoint. Only then
wire exact release-driven replay revisit and coordinator-enforced ACK reservation.

Pipeline pulse must retain replay-busy captures and select another eligible PID;
its current generic failure handling pops most results. Dispatcher contention
must not become a front retry plus delay that obstructs unrelated PIDs. Shutdown
must explicitly reconcile/release exact holds, or use a separately validated
atomic shutdown owner; close/discard loops cannot clear them. Existing claim
holders need drain/transfer admission while replay is pending.

Major-plan qualification remains deferred by user instruction. It must exercise
actual native overlap, all ownership handoffs, before-park release, checkpoint
uncertainty, shutdown/reinit, allocation refusal and unrelated-PID progress on the
combined source. Metadata inventories and this source review prove none of that.

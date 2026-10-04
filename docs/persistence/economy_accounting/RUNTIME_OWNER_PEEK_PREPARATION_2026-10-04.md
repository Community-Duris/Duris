# Pure runtime owner-cache revision observation — 2026-10-04

Status: **source implementation, UNQUALIFIED**. No tests/compiler/native/SQL or
service runs; new testing stays deferred until major-plan readiness.

Existing `item_ownership_runtime_owner_revision` hydrates an absent owner at
revision0. Calling it from a read-only graph proof can silently repair the very
missing metadata that should refuse verification. That source behavior is retained
for its existing callers; this is a new observation API.

`item_ownership_runtime_peek_owner_revision` validates the owner/output and performs
only the existing map lookup/copy. Missing/invalid/null refuses with output unchanged;
a stored revision0 remains a valid observation. No allocation, hydration, callback,
registry mutation or native authority follows. Its serialized game-thread caller
must establish thread access; the graph owner binds actual nevent game-thread state.
The cached global room revision can legitimately exceed selected entry revisions
and must never exceed separately current SQL authority. Per-entry acceptable
historical range is checked by the graph owner, not invented by this leaf API.

Major-plan cases still required: absent owner leaves cache/output unchanged,
invalid/null refusal, zero/nonzero observation, allocation failure arming without
allocation, reset/forget, selected intermediate cached revisions and stale/ahead
cache rejection with actual native graph authority. BEFORE API absence is not a
semantic RED. This is not native SQL/custody proof, an enrollment or an ACK permit.

## Frozen inputs

BEFORE selected files from7e8105d22 are retained; manifest `tmp/runtime-owner-peek-before-v1.local/manifest.json`
SHA-256 `34fb77b49474e6e727ab1d769d332f4d4cfa385bf10edef74217b3307f6480d2`.
Only source review, formatting, diff and hash inventory are allowed/executed here.

| Current input | Raw SHA-256 |
| --- | --- |
| `src/item/item_ownership_runtime.c` | `484f48570af98fa8756af49a64624b6920004b0f1aae1efa7e77bea87b68b2a2` |
| `src/item/item_ownership_runtime.h` | `6ec640f12f506bf5be8149d161eb7b18c3275268003b6dc0c48a234a484b2e42` |

Read-only architect review of these final pins found no blocker in the narrow
source scope. No native execution followed from that review. The combined graph
owner and all original qualification gates remain required.

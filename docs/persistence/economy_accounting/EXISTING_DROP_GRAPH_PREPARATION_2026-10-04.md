# Existing ordinary-drop graph observation preparation — 2026-10-04

Status: implemented source candidate, **UNQUALIFIED**. The user deferred native,
gameplay, persistence and recovery testing until the major plan is ready. No
compiler, test, AST check, SQL service or game journey ran for this slice.

## Requirement and source evidence

`ORDINARY_DROP_RECOVERY_INTERFACE_2026-10-04.md` requires an actor-independent
existing-graph observer before any recovery mutation. Per-UID lookup cannot
prove that an extra runtime descendant is absent. Historical retained-receipt
verification does not itself establish the current economic epoch. Returning a
decoded room graph after its transaction is released cannot preserve its lock
authority. The existing room publisher is a constructor and rejects an already
present expected UID; it is not the required observation owner.

The selected BEFORE inputs are preserved at
`tmp/ordinary-drop-existing-before-v1.local/manifest.json`, base
`7e8105d226a8cc2981149f0a4f1730b991255ece`, raw-byte manifest SHA-256
`b52cd1a93a12bcdd3b327fbcdf98c1b4ef32a6477556a0b4dbba65d0b692c229`.
This records 26 selected source/interface inputs and the absence of the new
module. It is not a compiler dependency closure. A missing API is not an executed
semantic RED.

## Owned implementation

`src/item/ordinary_drop_recovery.c/.h` adds
`ordinary_drop_recovery_observe_existing(original_command, sealed_completion)`.
The generic header has no SQL types. The result distinguishes
`verified_existing`, `absent`, `conflict`, `unsupported`, `refused` and
`unavailable`, with bounded error/witness/revision metadata. The returned value
is an observation at this call, **not a publication ACK capability, construction
permission, retained lock or reusable SQL proof**.

The supported scope is publication-required schema-2 accounted ordinary
player-to-room drop, with one existing root and a complete literal payload.
Current payload validation retains the existing room sidecar restrictions;
there is no source adoption, new UID, money/corpse/locker/pet/peer-give route,
continuation or detached constructor. The observer requires the original
operation ID, execution disposition, definitive successful completion, exact
canonical 48-byte result, expected owner revisions plus one, the maximum item
revision plus one, and the native durable revision
`max(from_owner_revision, to_owner_revision, max_item_revision)`.
Applied/already-applied delivery outcomes are equivalent; attempt/timestamp
metadata does not change the retained result. A changed canonical receipt cannot
establish observation authority. Retained comparison binds the whole fixed
native result array, including bytes beyond its declared body size.

The SQL branch refuses calls outside the already-bound game thread before pool
or world access. It immediately owns the acquired pooled connection with
`player_sql_pool_lease`, refuses nonidle/autocommit-off/reconnect-enabled
sessions, and marks cleanup pending before the literal `START TRANSACTION`.
No per-call `mysql_thread_end` is introduced on the game thread.

Within one session the order is current economic lineage/epoch lock,
current season/room-owner/custody/payload read, retained original inbox and
native economic/outbox verification, then native graph comparison. The shared
inbox verifier uses a nonlocking receipt read, avoiding a late inbox write lock.
Session identity, transaction status and disabled reconnect are checked before
and after native comparison. Every attempted transaction exits through explicit
original-session rollback confirmation. Uncertain rollback/session cleanup
overrides even a successful observation with `unavailable`; its lease retires
instead of being returned reusable. There is no replacement-session proof reuse.

Current SQL item UIDs/vnums/root/parent/owner/state and revisions must match the
original drop exactly at expected item revision plus one. Current room revision
may exceed the original drop revision. Per-item canonical blobs are compared by
UID, independently of command-entry sorting versus snapshot tree order.
Only serialized parent index and equipment slot are normalized; literal text,
generated key, weight, timers, flags, values, affects and descriptions retain
their existing canonical codec meaning. Null/empty string identity follows the
native snapshot codec rather than raw string-pointer identity.

Physical proof walks the complete global object list, requires one global
occurrence per expected UID, scans room root lists for sole intended placement,
and captures the selected tree with `player_item_snapshot_tree_capture_literal`.
Every global parent's containment list and every global character's carrying
and equipment links are also checked before absence or success. Foreign-parent
and character aliases cannot be hidden by correct location fields. Every
reachable object link must refer to a registered global object pointer; an
unregistered intermediate refuses instead of hiding a deeper selected UID.
Per-list generation marks detect cycles without following unregistered links.
Separate reciprocal containment/pointer checks reject a nonglobal lookalike
child having the same UID and bytes. Extra, partial, duplicate, misplaced,
changed, cyclic or nonreciprocal graphs refuse unchanged. Runtime root census
includes foreign owners/states and extra UIDs; individual expected-UID lookups
also catch rows claiming a different root. Per-entry owner revisions may lie
between the original drop revision and current SQL room revision. The pure
owner-cache peek requires its revision to cover every selected runtime entry
without exceeding SQL authority; it never hydrates a missing owner.

Global object nodes, room traversal, containment links and character/equipment
traversal consume one shared 1,000,000-step budget; the remaining budget must
also cover both full registry census passes. Repeated corrupt lists cannot
multiply work outside that budget. Global-pointer and per-list-mark vectors
are bounded by the counted global nodes; their storage grows with world size.
Sorting/lookup adds bounded comparison work. Exhaustion is `unavailable`, never
a truncated absent result. Native capture retains its object/depth/byte bounds.
Runtime census allows one extra row beyond the maximum admitted transfer forest
to identify an ordinary extra descendant; larger/opaque census failure remains
unavailable. This does not impose a new live-world write permission or mutate
the registry.

## Deferred qualification preparation

`tmp/ordinary-drop-existing-prepared-v1/cases.md` is a declarative native case
plan with 44 unexecuted cases, not an executable runner or observed result. The
production module must
be linked with the actual native codec/capture/runtime and SQL receipt/current
payload owners. Both real disposable MySQL and MariaDB engines, actual pooled
lease cleanup and real native globals are needed. Controlled world inputs can
establish this observer's unchanged behavior; they cannot qualify real handler
publication, crash recovery, critical ACK ordering or the full ordinary-drop
route. No deadline increase is proposed.

## Remaining integration and authority limits

Primary owns shared read-only retained-receipt and pure runtime-peek seams,
Makefile registration and their independent source review. Shared validators
that hide allocation failures behind booleans/codecs require conservative
unavailable classification. Direct capture/current-payload/authority refusal
remains unavailable where malformed and OOM cannot be distinguished; retained
retryable/ambiguous outcomes remain unavailable independently of errno. Explicit
canonical result/native byte/UID/topology discrepancies still produce conflict
or refused. New canonical encoding explicitly preserves allocation failure.
This source preparation does not claim exhaustive preexisting helper allocation
qualification, live database cost or full compiler closure.

Primary must place this observer before the existing item-movement runtime
mutation, add startup/dispatcher ownership and actor-independent routing, and
retain the original publication obligation through critical ACK cleanup. A
matching graph here grants no permission to skip those boundaries. All-absent
construction, inert-stage release/enrollment, global reservation, atomic
hydration/publication, complete cold restart recovery, full source census and
production accounting activation remain separate open work. No production data,
inactive behavior, generic loader or declined spell path was changed.

## Source checkpoint

Final owned source raw-byte SHA-256 pins:

| File | SHA-256 |
| --- | --- |
| `src/item/ordinary_drop_recovery.c` | `87b5a888f72d87c95f56ec32a736c3b6ab40b8998dbff490b2ac1f735d85e9d9` |
| `src/item/ordinary_drop_recovery.h` | `f9cf986aec1d3e7dd1df8e218b75c10bcc0251cd5abfc64eff6e019ddd3228cd` |

The independent architect source review identified and corrected foreign
container/character aliases, opaque proof-error classification and whole-array
receipt identity. This review is separate from qualification.
The final full-module architect rereview found no remaining source blocker for
this narrow observation-only scope at the two source pins above. No execution
or recovery qualification was claimed. Formatting used
`clang-format-18 -i` only on the new source/header. Raw hashes, whitespace/diff
inspection and private selected-input snapshots are source preparation, not
tests. The AFTER selected-input manifest is
`tmp/ordinary-drop-existing-prepared-v1/manifest.json`; it must not be described
as a full consumed compiler closure or a clean published HEAD.

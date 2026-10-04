# Ordinary item publication receipt preparation — 2026-10-04

Status: **IMPLEMENTED, SOURCE-REVIEWED, UNQUALIFIED**. No compiler/native/
SQL/gameplay/recovery run or milestone push occurred. Source review established
that ordinary completions could overwrite semantic proof after projection or
while the original publication ACK was pending; only craft protected that phase.

`item_movement_transaction.c` now seals well-typed definitive ordinary receipts
in batch validation before collector/registry/physical effects, and seals definitive
rejection before its callback/notification. Uncertain results and malformed success
remain repairable before publication. Unknown outcome/failure-stage enums and
oversized payloads refuse before decode. Semantic equality binds original operation,
disposition, revision, error, failure stage, size and full bounded result image;
applied/already_applied equivalence and delivery attempt/timestamp differences remain
allowed. Same-batch contradiction cannot be cleared by a later matching duplicate.

An inflight latch suppresses recursive publication while allowing recursive delivery
to retain a contradiction. After callback, the owner re-resolves its key and checks
the stack-frozen original receipt before accepting proof. A successful callback sets
an independent ordinary_publication_ready latch before the blocked check. Receipt
conflicts may change the diagnostic enum phase, but cannot discard completed
physical publication: canonical repair selects original ACK/notification only,
without repeating registry or physical callback. Exceptions reset inflight state,
retain the owner and do not claim publication readiness.

Independent architect review initially found that receipt sealing alone lost
ACK-pending progress across a conflict. The readiness latch/ACK-only dispatch fixes
both callback-success→ACK-failure→contradiction→canonical-repair and reentrant
contradiction during a callback that returns success. Focused final source review
found no remaining blocker. Craft progression and unretained legacy paths remain
separate; healthy inactive spell paths were not changed.

Final source SHA-256: `5915bdfda668ea843319709fde3fd8d9b118703c2cbe7c16fe9e878bdf4ec515`.
BEFORE selected-source base80692d52b:
`tmp/item-receipt-sealing-before-v1.local/manifest.json`,
SHA-256 `0e139cb096b8b091f347e929de0fa790970a2bc7c24d98a04febf5cd70bc80fa`.
Private owner `tmp/item-receipt-sealing-prepared-v1/` prepares unchanged30craft
regressions plus a separate ordinary public-API family with real registry,
coordinator and journal. Craft custody/world leaves remain controlled; neither
family establishes native SQL authority, actual game handler or full cold recovery.
No missing new helper/API compilation may be called semantic RED.

Full actor-independent room publication, inert materialization, exact native graph/
epoch proof, mutation census, critical ACK reservation, deferred wake and production
startup remain in the [recovery handoff](ORDINARY_DROP_RECOVERY_INTERFACE_2026-10-04.md).
Qualification must retain original budgets and actual gameplay/persistence/recovery
checks at the major-plan milestone. R1–R8 and release=BLOCKED remain.

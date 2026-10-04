# Transaction-scoped ordinary-drop inbox proof — 2026-10-04

Status: **source implementation, UNQUALIFIED**. Testing remains deferred until
major-plan readiness. No compiler/native/SQL/gameplay/service/recovery check ran.

The existing reconcile API opens/rolls back its own transaction; the creation
verifier admits a different route. Neither can prove an ordinary retained drop
inside the graph owner's continuously held native authority.

The new `critical_command_repository_verify_ordinary_drop_in_transaction` accepts
only successful schema2 ordinary SQL-drop proof with exact retained literal
capture. It reads the committed inbox without a late FOR UPDATE, verifies the
original command/key hashes/type/versions and typed result UID/count/revisions,
then verifies retained canonical economic root/payload and outbox. Owner/item
revisions advance exactly once; durable revision is the native maximum of original
source owner, destination owner and item revisions. Later room revision advancement
belongs to the separate current graph proof, not this historical receipt.

Caller must acquire current original lineage, season, room, custody and payload
locks in native order before calling: historical verification also locks payload
rows. No START/COMMIT/ROLLBACK, SQL write, missing-command apply or publication ACK
occurs here. A reconnect-disabled session and active transaction are checked across
proof. The caller owns cleanup and must refuse a positive observation if exact
session rollback/idle proof or lease retirement cannot be established.

Missing/uncommitted inbox returns retryable EAGAIN; changed identity returns EEXIST;
explicit typed result corruption refuses EILSEQ. Existing boolean support validators
and payload codecs hide some allocation failures; opaque refusal retains retry
instead of manufacturing a rejection. The inherited inbox reader also combines
some malformed-row failures with unreported SQL errors; the fallback may be ENOMEM.
This API does not claim to distinguish every corrupt row from temporary resource
failure. Any unproven result remains held. Flatfile returns ENOTSUP.

Major-plan cases still required: exact original command/result and full body,
changed request/hash/key/type/version, missing/uncommitted/rejected inbox, malformed
root/plan/source/reference/outbox, high owner versus item durable revision, uint64
revision boundaries, later epoch/custody/room changes, each allocation refusal,
reconnect/session loss and actual current-lock ordering with both SQL engines.
New API absence on BEFORE is unsupported, not semantic RED. Current producer,
coordinator/save census/ACK reservation and cold startup integration remain open.

## Frozen inputs

BEFORE selected files from7e8105d22 are retained; manifest `tmp/ordinary-drop-inbox-before-v1.local/manifest.json`
SHA-256 `5fcc2cc2b09ca5e3095028255bfc38250f405c6cb1fd28d543e99ba28f722af7`.
Only source review, formatting, diff and hash inventory are allowed/executed here.

| Current input | Raw SHA-256 |
| --- | --- |
| `src/persistence/critical_command_repository.c` | `ca3fd630e82268693cbd1667b8d59cbea8746a0602bdc2d85315487470222423` |
| `src/persistence/critical_command_repository.h` | `6b4988d03f4081eb9136e9bfc6458a49ccf554b2ce9ac37ffe5ecee58ecbaf8a` |

Read-only architect review of these final pins found no blocker in the narrow
source scope. No native execution followed from that review. The combined graph
owner and all original qualification gates remain required.

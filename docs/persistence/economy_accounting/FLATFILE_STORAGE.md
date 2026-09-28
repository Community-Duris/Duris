# Flatfile retained accounting storage

Status: storage foundation for #478; gameplay integration and activation are
unfinished. The SQL and flatfile APIs share canonical EAI1 intent and EAP1 plan
bytes. A structural record is never proof that a domain mutation was authorized
or applied. The critical coordinator remains the only admission/retry mechanism.

## Atomic publication and ownership

The evidence directory is `FLATFILE_ROOT/economic-evidence`, owner-only. Store
number 7 extends the existing v2 authority journal without changing earlier
store numbers or v1/v2 framing. Generic runtime commit rejects this reserved
store. Only private staging/commit methods, accessible to future typed bank and
lifecycle owners, may publish its after-images. The test-only friend is absent
from production builds. Recovery can replay reserved operations from its
checksummed journal. An older binary must refuse an unknown store and retain the
journal; it cannot safely run against newly activated accounting state.

A domain owner must acquire the shared authority lock, recover authority and
legacy domain journals, look up the admitted operation ID, prepare/verify actual
locked domain effects, stage the exact domain and accounting bundle, commit,
then publish the retained result. Storage calls recover the authority journal
before reading; they do not establish domain-write authorization or replace the
owner's initial recovery/revalidation. Missing/corrupt state is unresolved, not
permission to acknowledge or apply another operation ID.

Each append adds exactly two journal operations: the active/new segment and its
bucket index. Thirty domain after-images therefore still fit the 32-operation
limit. Preflight includes the actual 256 MiB journal limit, 50-byte framing and
8+filename-length bytes per operation. Future typed adapters must also bound
their combined maximum domain after-images; a 256 MiB world catalog leaves no
room for accounting. The storage bridge does not silently split one root commit.

## Record version 2 and file framing

All integers are explicit little-endian. Each file/record has a 48-byte envelope:
8-byte magic, u32 version 1, u32 payload length, and SHA-256 of the entire payload.
Magic/version/length must exactly match the expected layout. Reserved fields are
zero. Decoders bound lengths/counts before allocating and retain caller outputs
on error.

- Record magic `DURECR2\0` (the earlier unshipped prototype is refused). Its payload starts with u32 command/plan/result byte
  lengths, u32 result code, u64 durable revision and u16 failure stage, followed by those byte
  strings. It retains the exact canonical schema-2 admitted command (including
  intent and admission timestamp), canonical plan and original result. Success
  requires a structurally valid plan whose complete metadata matches the frozen
  intent; rejection requires a nonzero result code and no realized plan.
  Failure stage must be a defined value and must be none for success. Child-bearing plans are refused until child-ID reservation is implemented. Domain-specific result/actual-effect verification remains the typed owner's job.
- Index `bucket-XX.eai`, magic `DURECI1\0`. Payload: lineage[16], u32 bucket,
  u32 entry count, u64 total record bytes, then sorted 64-byte entries containing
  operation ID[16], record SHA-256[32], u32 segment/offset/size and u32 reserved.
  Operation IDs are unique and their first byte identifies the bucket.
- Segment `bucket-XX-N.eas`, magic `DURECS1\0`. Payload: lineage[16], u32 bucket,
  u32 segment, u32 record count, u32 reserved, then contiguous complete records.
  Index offsets are relative to the first record. Every indexed range must be
  contiguous, nonoverlapping and within the exact segment payload; each record
  digest must match. Segment numbers are dense and start at zero.

The active segment grows by retaining its entire old record prefix and appending
one record. It rotates before exceeding 8 MiB; sealed segments are never rewritten
or pruned. The corresponding index retains every prior entry. Both after-images
are published by the authority journal together with domain state and receipt.

## Bounds and stale-state refusal

There are 256 buckets, at most 4,096 records and 256 MiB record bytes per bucket.
A record is bounded by 48+26+512 KiB+4 MiB+4 KiB = 4,722,762 bytes. The derived
format limit is 74 segments per bucket. The index maximum is 262,224 bytes.
Aggregate upper bounds are 1,048,576 records, 64 GiB record payload and 19,200
segment/index files; an individual bucket can fill sooner and framing adds disk
space. These are capacity limits, never eviction or a rolling retention window.
At capacity new operations fail while retained lookup remains available.

Every lookup, including an old ID or absent ID, verifies the active segment and
requires the next segment name to be absent. This detects a stale valid index
whose old active segment was subsequently sealed. A stale index within the same
segment fails exact coverage/digest checks. An older selected segment is verified
before returning its record. Missing indexes or indexed segments, unsupported
versions, wrong lineage/bucket/number, checksum mismatch and corrupt pending
journals refuse access. Per-lookup reads are bounded by one index and at most two
8 MiB segments; staging performs bounded copies, never a scan of world history.

Private bucket initialization checks an existing private directory and refuses
any matching bucket files. It is not callable by gameplay. Before activation,
the lifecycle owner must additionally prove durably that the bucket was never
activated; absence alone cannot distinguish fresh state from lost history. All
required buckets and lifetime/epoch mappings must be initialized consistently.
No automatic missing-index reconstruction, empty reset or compaction is provided.

## Retention and remaining integration

Lifecycle entries protect indexes and segments through season reset and restore.
The existing managed backup recursively captures both classes under the shared
authority lock; its existing metadata/disk budgets still apply. Native restore
qualification must gain a full semantic store scan before activation; capture
coverage alone does not prove restored accounting consistency.

The existing 512-receipt player cap and other bounded domain receipts remain.
A typed flatfile adapter must atomically connect actual domain after-images,
retained accounting lookup and exact result, then safely adapt those hot receipts
without losing old replay fences. Source claims, retained lifetime/epoch metadata,
compound savepoints, reconciliation/baseline/export tooling and release writer
coverage remain unfinished.

## Storage qualification

The reused native ASan/UBSan harness exercises syscall fault/process-exit cases across
both initial commit and journal recovery: short and interrupted writes, zero
writes, ENOSPC, file data sync, rename, directory sync and journal removal.
Failures do not acknowledge success or overwrite the lookup result. A clean
retry recovers the complete domain/evidence bundle or proves the journal was
never published, then retries the original ID once. Repeated lookup preserves
exact result/plan bytes and does not append another event. These tests model
process exits and syscall failures; they do not simulate storage power loss.

Other cases cover canonical/corrupt/unsupported bytes, stale indexes, wrong
locks, operations older than 512 later receipts, segment rotation, full retained
indexes, and exact 32-operation/256 MiB journal limits. The byte-boundary test
stages a synthetic large image in memory without publishing it. These storage
tests do not qualify the future gameplay adapter or complete slice 04.

## Current extraction

Based on PR #604, this increment reuses `42cdf47c1` with selected durability
fixes from `6631c4e9b`, `411d8102` and `0d8e6bb3`. A pending journal must
block a second commit without recovering already-prepared after-images. Allocation
failures preserve lock reuse and return I/O failure with ENOMEM; authority files
with multiple hardlinks are refused. This does not import authority checkpoint v3,
root descriptors, legacy indexing, compound reservations or baseline activation.
The new receipt preserves the current 4096-byte completion limit and failure stage;
legacy player-domain receipt limits remain independent.

The expanded ASan/UBSan suite passes the 85 original commit/recovery fault cases
plus pending-journal overwrite refusal, reusable allocation-failed locks, encoder
ENOMEM classification, hardlinked index/segment/lock refusal, canonical child-plan
refusal, failure-stage roundtrip and full 4096-byte result retention. Existing
authority/player-domain/account, lifecycle, backup and provisioning tests pass.
Both full server builds (flatfile and MariaDB) pass. Hosted qualification and review remain pending.

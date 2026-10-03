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
bucket index. The authority transaction limit is 4,099 operations, enough for
4,096 player spell receipts, a player snapshot, a death disposition, and custody. Preflight includes the actual 256 MiB journal limit, 50-byte framing and
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


## Player spell application receipts

These are publication recovery evidence rather than double-entry postings. Each
`FLATFILE_ROOT/players/<pid>-<operation-id>.spell` file binds one positive player
PID, one 16-byte operation ID, its effect ID, and the committed player revision.
The operation ID uses 32 lowercase hexadecimal characters. The file reuses the
checksummed DURPLYR version-1 envelope and a minimal schema-12 snapshot containing
exactly one spell receipt. It is never materialized as gameplay or inventory.
No additional journal store number or file-envelope version is introduced.

Receipt-bearing saves commit the immutable receipts and player state through the
existing authority journal under the player lock followed by authority. Death
saves include their immutable disposition and custody quarantine in that same
transaction. Ordinary saves and player loads recover this journal before reading
or advancing the player file. Exact replay requires the requested receipt and
matching effect ID; death replay also compares the immutable disposition bytes.
Read I/O failures remain retryable, while missing or contradictory replay evidence
fails permanently without acknowledging the effect. A receipt newer than the
prior durable player snapshot is inconsistent evidence and cannot authorize a
later save.

A load reads only receipt operation IDs requested by pending spell publication.
It does not scan retained history; an unrelated receipt cannot consume the
4,096-operation request budget. Later ordinary saves keep the immutable files
and omit receipts from the materialized player snapshot. The lifecycle manifest
registers `players/*.spell` as protected recovery state, retained through the
controller-approved recovery horizon. The player tree is included in flatfile
state backups. The restore qualifier checks every retained receipt, its filename
identity, and its revision against the restored player. Deleted players may retain
receipts only with their inactive identity tombstone. This does not establish
whole-backup rollback qualification.

`test_flatfile_player_repository.py` covers refused commits, interrupted
receipt/player publication, recovery in a separate process, exact replay,
missing/conflicting/corrupt receipts, injected read I/O failures, a restored
player behind its receipt, and death recovery. Ordinary saves can also carry quest
XP receipts in the same transaction, as described below.

## Player quest XP receipts

The existing `domains/item_ownership` catalog now writes format 8 and reads
formats 1 through 8. Quest continuations are validated with the complete version
1–5 decoder, including their frozen recipient awards. Format 7 appends a u64
application mask to each retained operation, followed by one u64 player revision
for each set bit, in increasing bit order. Version-5 bits identify frozen award
slots; version-4 bits identify solo XP reward slots. A mask cannot reference an
unallocated XP slot, and a stored revision cannot be zero. Earlier formats have
no application markers; acknowledgment does not reconstruct missing markers.
Older binaries cannot read format 8. Rollback therefore requires the matching
pre-upgrade state and binary together.

An ordinary schema-11/schema-12 or death schema-15 save verifies each receipt's offering ID,
recipient PID, reward index, and frozen amount under the authority lock. It must
include the experience status field. The updated catalog and materialized player
snapshot commit through one authority journal. Death saves merge XP markers
and custody quarantine into one catalog after-image, preventing either update
from overwriting the other. Their immutable disposition is part of the same
transaction. A combined spell/XP save adds its
immutable spell files to that same transaction. The materialized player file
does not retain a bounded receipt list. Exact-revision replay requires every
requested application marker, while stale saves do not acknowledge receipts.
Existing markers must not exceed the player's durable revision; requested receipt
saves and gameplay loading refuse or flag a player restored behind its marker.

Owner recovery receives a reward-index XP mask. Peer recovery receives each
unpaid frozen award, including awards left pending after the offering owner
acknowledges its own rewards. New owner acknowledgment requires its own durable
XP markers. The existing protected ownership catalog retains both continuation
and markers; no separate store or lifecycle entry is introduced. XP receipts are
progression evidence and do not create economic ledger postings.

The player repository harness covers legacy version-4 solo rewards, current
version-5 solo/group rewards, changed amounts and foreign recipients, missing
markers, refused and interrupted commits, recovery in a fresh process, injected
catalog read failure, exact replay after acknowledgment, stale replay, a restored
player behind its marker, mixed spell/XP saves, and peer recovery after owner
acknowledgment. The item harness verifies version-5/version-6 catalog read/replay
compatibility and retained version-1 item reward obligations.

The recovery map retains applied XP identity before requesting a save. Failed
capture/admission leaves it pending, and later ordinary/death snapshots collect
the receipt plus status, trophies, and any coupled skill component. The bounded
64-receipt capacity is checked before an additional live XP application.
Death requests use schema 15 and conflict evidence uses schema 16; optional
spell receipts share those frames. Older ordinary/death encodings are unchanged.
Native journal/worker replay requires the exact successful revision, and the
terminal entrypoint retains the original receipt across timeout/resume.

The player harness now also covers mixed XP/spell group death, interrupted
publication, fresh-process recovery, and exact custody quarantine alongside the
XP marker. The live quest harness qualifies group admission and a linkdead
recipient on both maintained backends; the flatfile group-XP restriction has
been removed. Full service restart, copyover, save/custody races, staging-derived
restore, and whole-backup rollback qualification remain pending.

## Quest item and cash delivery proof

Format 8 appends 17 bytes after each ownership operation's XP revisions:
the original creation source ID (u64), recipient PID (u32), item VNUM (i32),
and a one-byte legacy quest-history flag. Successful single-root sourced
system-to-player creations retain the original delivery identity alongside their
existing result UID. Subsequent custody movement does not erase that delivery.
The load path derives each frozen quest slot's stable source ID and verifies
one native creation, its original recipient/VNUM, and the retained UID entry.
Ambiguous or inconsistent records fail without publishing a partial mask.

Player-domain format 4 reads formats 1–4 and retains the existing 2,048-byte
result and 64 KiB file limits. It appends eight bytes to each operation:
one-based quest reward index (u32) and canonical copper amount (u32), both zero
for an unproven operation. Only a successful recovery-site wallet reward with
zero bank deltas and an exact positive canonical denomination vector records
this proof. The bytes share the original native wallet/bank commit and its
operation result. Cold load uses the deterministic child operation ID and checks
the frozen slot/amount and decoded result before marking the cash delivered.
Exact command digest replay remains unchanged.

The player-load result carries a verified economic slot mask into quest recovery.
Recovery skips paid slots before creating an item or submitting a cash command.
Catalogs older than format 8 cannot prove their original item deliveries; pending
economic rewards from them carry a persistent review hold. A rewrite to format 8
preserves that hold. A matching cash operation without the format-4 proof also
holds recovery. Gameplay displays the pending ownership/payment review message;
current inventory and balances do not clear it.

Rollback after either new format has been written needs a matching pre-upgrade
state and binary. Existing receipt capacity bounds remain enforced; this change
does not remove the legacy player-domain receipt/file ceiling or qualify a
whole-backup rollback.

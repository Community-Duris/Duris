# Flatfile accounting authority metadata

Status: retained metadata and private lifecycle staging foundation for #478/#479, based on PR #605.
No production baseline, lifecycle owner, domain activation or gameplay admission
is implemented by this module. Structural metadata is not evidence that a native
account creation, deletion, rename or opening balance was authorized or applied.

## Identity and transaction boundary

`flatfile_accounting_authority.{h,c}` borrows the existing authority lock and
recovers its journal before every read or stage. The future lifecycle owner must
acquire other native locks in established order, recover legacy domain journals,
verify the real native effects and append its exact operation receipt. Metadata,
native after-images and receipt must commit together through the private authority
bridge. These helpers never publish, retry or commit on their own. Generic
runtime authority commit still rejects reserved store 7.

Version 1 supports wallet PID locators and shared-bank name/racewar locators.
Wallet context is zero; bank context preserves the supported nonnegative signed
byte racewar range. Canonical bank names are at most 50 lowercase ASCII letters,
digits, underscores or hyphens. Names are locators, never account identities.
The allocated mapping ID is the bank's retained native lifetime as well as its
account authority ID. Wallet mappings retain their original positive int32 PID.
Other account/locator kinds fail closed until their lifecycle adapters are added.

All mapping IDs are sequential from one and never reused. Retirement retains the
full original mapping and records its retirement operation. Recreation at the
same locator allocates a new ID. Rename preserves mapping ID, native ID,
lineage, kind/context and creation operation; it changes the current bank name,
mapping revision and last operation. Both old/new native indexes, mapping bucket
and control digest change atomically. A same-bucket rename emits one merged index.
An old alias can point to a still-active mapping now using another name. Its
retained last mapping must exist and have matching immutable locator scope;
a tombstone cannot hide a mapping still active at that same name.

## Version 1 files

All files live in private `FLATFILE_ROOT/economic-evidence` under reserved store 7.
Every file has an eight-byte magic (including terminal NUL), u32 version 1, u32
payload length and SHA-256 of the payload, followed by little-endian payload.
Reserved bytes must be zero. Exact length, checksums, identity, ordering, counts
and crosslinks are checked before returning data. Unsupported files are refused.

- `authority.eal`, magic `DURECA1`: lineage, creation and last operations, last
  and active epoch (five 16-byte IDs); u64 revision and next mapping ID; u32 epoch
  count and reserved; epoch-catalog digest; 256 native and 256 mapping digests;
  32-byte evidence-bucket initialization bitmap. Total size is 16,600 bytes.
- `epochs.eae`, magic `DURECE1`: lineage, u32 count/reserved, then 96-byte entries:
  epoch ID, u64 ordinal, predecessor ID, u16 transition kind, six reserved bytes,
  transition digest and creating operation ID. Entries are never removed or
  changed by the staging API. IDs are unique, ordinals consecutive and each
  predecessor matches the previous entry. Appending an epoch clears selection;
  selecting/pausing the latest epoch is a separate private lifecycle operation.
- `mapping-XX.eam`, magic `DURECM1`: lineage, u32 bucket/count, then u32-length
  entries containing canonical 40-byte account key, u16 locator kind/name length,
  u64 native ID, name, three 16-byte creation/retirement/last-operation IDs and
  u64 mapping revision. A bucket retains exactly the allocated IDs where
  `mapping_id % 256 == XX`, in ascending order. Holes and unexpected IDs fail.
- `native-XX.ean`, magic `DURECN1`: lineage, u32 bucket/count, then entries with
  u16 key length/reserved, u64 active/last mapping IDs and canonical native key.
  The key is u16 account kind, u64 context, u16 locator kind, followed by a u64
  wallet PID or the canonical bank name. The first SHA-256 byte selects the bucket.
  Keys are sorted and unique. Active entries require active=last; tombstones
  retain last with active=0. Selected entries are checked against retained maps.

The control file binds the exact full-file digest of every initialized child.
A zero native digest means never initialized, and an unexpected file is corruption.
A zero mapping digest is allowed only before the first ID assigned to that bucket.
Allocated buckets cannot be missing, reset or silently rebuilt. A valid older
index/catalog/mapping file fails its current control digest. Control and changed
children are always staged in the same recovery bundle.

There are at most 1,048,576 mapping lifetimes (4,096 per bucket), 4,096 native
locators/tombstones per bucket, 4,096 epochs and 514 metadata files. Maximum valid
mapping/native bucket sizes are 663,624/335,944 bytes; the epoch catalog is at most
393,288 bytes. A defensive 2 MiB file-read limit applies. Capacity refusal never
evicts a mapping, alias or epoch. Rename into a full new-locator bucket leaves
the original alias intact. Revisions cannot wrap. Journal staging preflights
32 destinations, duplicate destinations and exact 256 MiB framing.

## Reads, initialization and limits

Cold native lookup returns a currently active mapping. Retained lookup uses the
immutable account key and does not require current activity, selected epoch or
historical command name to match the mapping's current name. Fresh authority
snapshots require the exact selected epoch, current native identity and active
mapping/index crosslinks. Up to 3,072 requests are grouped by mapping bucket and
then native bucket; each required bucket is read once. Peak decoded storage is
one bucket plus the requested results and bounded grouping indexes, not all
retained mappings. This is a structural I/O bound; release latency/contention
qualification remains pending.

Bootstrap is private and refuses any existing evidence-directory contents, but
that absence check is insufficient proof of never-activated state. A future
quiesced lifecycle owner must establish the external durable virgin-state or
approved upgrade proof before calling it. Native-index initialization is tied to
control digests; evidence-index initialization atomically sets its control bit.
Repeated evidence initialization verifies its index/active segment before
reporting EALREADY. Allocation exhaustion stays distinct from corrupt storage.

Root digests detect inconsistent child restoration. They cannot detect coherent
rollback of the control file and all matching children. Verified restore,
lineage/epoch transitions, source coverage and cutover must establish that wider
boundary. Backup capture includes all four metadata classes, but full semantic
restore scanning and maximum backup-budget qualification are unfinished.

The existing legacy player receipts remain unchanged (512-entry and 64 KiB limits).
Typed schema-2 ATM will use the segmented receipt store and preserve old receipts.
Before deletion can discard a legacy domain file, a tagged indexed archive must
retain its original ID, hash and result without inventing historical plans.
Native creation/deletion/rename integration, baseline openings, SQL lifecycle
parity, per-domain coverage gates and all remaining #474 adapters are pending.

## Executable qualification

`tests/async/test_flatfile_accounting_authority.py` builds an isolated native
ASan/UBSan executable. It covers lifetime allocation beyond one bucket cycle,
same/cross-bucket rename and rename-back, alias recreation, retirement, retained
lookup, epoch pause/selection, stale valid child files, contradictory tombstones,
missing allocated records, unsupported versions, wrong locks and exact operation
bundle refusal. A multi-account snapshot counts real file opens to verify each
bucket is read once. Linked C++ allocation failures preserve outputs, including
repeated initialization; full native-index and revision limits refuse safely.
Ten process exits cover each create/rename/retire metadata publication boundary.
These are synthetic storage journeys, not native-account lifecycle or power-loss
qualification. The existing 85-case storage syscall/crash suite remains separate.

## Current extraction and verification

Reuse `6698326e4` plus the retained epoch membership read from `d47c7af0b` and
`c9107c082` allocation-error preservation. Keep the phase6 v2 authority journal
and its 50-byte framing; no checkpoint-v3 or inspection API is imported.
Retained epoch lookup verifies the complete catalog and lineage and permits
historical epochs without requiring current selection. These metadata envelopes
do not change the DURECR2 receipt codec, failure stage or result limits.

Lifecycle, managed backup and contract/census checks pass. Authority and storage sanitizer qualification passes, including exact metadata
read ENOMEM/EIO, historical epoch membership, 10 authority crash boundaries and
85 storage fault cases. Both full server builds (flatfile and MariaDB) pass. Hosted qualification and review remain pending. Gameplay and native
account lifecycle effects are not established by metadata fixtures.

# Flatfile baseline retention

The private `flatfile_accounting_baseline_storage` stage retains a complete EAB1
witness, EAI1-bound opening plan, original common accounting receipt and per-epoch
account/item reservations in one existing shared authority transaction. It never
mutates native balances, allocates UIDs or writes a second item custody ledger.
Only the future lifecycle transaction owner may invoke it in production. No
current command admission, native source proof or domain activation is added.

## Authority obligations

The lifecycle owner must authenticate the operator and establish a frozen native
boundary, verify complete source coverage and exact mappings/revisions, drain
unresolved/publication work, and retain lifecycle/initialization receipts. The
storage seam alone does not prove those conditions. Initialization requires a
retained lineage/epoch and refuses any existing file in its baseline namespace;
absence still cannot prove never-initialized state after an incomplete restore.

A prepared witness is not an activation capability. The caller must not expose
this stage to arbitrary command input or accept caller-supplied source hashes as
proof of native state. Backend selection, per-domain coverage, safe pause and
activation remain pending. The SQL baseline storage counterpart is also pending.

## Files and bounds

All files are in private `economic-evidence` (reserved store 7), share the existing
authority lock/journal and are protected lifecycle records. Names begin with
`baseline-<lineage hex>-<epoch hex>-`:

| Suffix | Contents |
| --- | --- |
| `head.ebc` | DUREBC1 envelope; lineage, epoch, opening account, revision, last operation and 16 index hashes |
| `<hex bucket>.ebi` | DUREBI1 envelope; lineage, epoch, bucket, count and sorted reservation rows |
| `<operation hex>.eab` | Complete canonical EAB1 witness, bound by the original command |

Control/index envelopes use 8-byte NUL-terminated magic, little-endian u32 version
1 and body length, and SHA-256 of the body (48-byte header). Control size is 656
bytes: lineage/epoch 16 bytes each, opening key 40, revision 8, operation 16, then
16 full-file index hashes of 32 bytes. Index body starts with lineage/epoch,
u32 bucket/count, followed by 32-byte rows (u64 kind, u64 identity, operation16).
Kind 1 reserves an account lifetime across all holding kinds and contexts; kind 2
reserves an item UID. These namespaces are separate. Buckets use identity modulo
16, sort by kind then identity and refuse duplicates and noncanonical identities.

Each index holds at most 65,536 rows (2,097,240 bytes), for 1,048,576 reservations
per epoch if distributed across all 16 buckets. A full bucket refuses further
openings before any write, even if other buckets have room. Merging is once per
bucket, avoiding repeated shifts of a full retained index. Initialization scans
at most 1,048,576 directory entries and refuses an exceeded scan bound.

Initialization stages 17 after-images. A batch stages at most 20: sixteen changed
reservation indexes, one witness, one book head, and the existing common receipt
segment/index. Complete maximum witnesses (3,071 holdings and 6,000 item records)
fit without splitting forests or raising the shared 32-operation limit. Existing
journal byte/count budgets also include caller-supplied lifecycle after-images.
Duplicate operation targets refuse the whole stage. Empty batches retain an
explicit receipt/witness and advance the book using four after-images.

## Replay and integrity

New batches first resolve the original operation ID in the common indexed store.
An exact retained receipt must pass complete witness regeneration, exact plan
comparison and selected reservation membership before returning `already_exists`.
Changing receipt identity/time conflicts. Changing an account's kind/context or
using another preparation ID cannot reserve the same account lifetime again.
Zero-valued holdings and unchanged items are reserved as well.

New writes validate all sixteen index hashes and bounds. Retained reads validate
the book, full witness and all indexes touched by that original batch, independently
of the current active epoch. Missing book/index/witness, orphan witnesses,
checksum-correct reservation-owner changes and plan mismatches refuse. Read APIs
may replay an already durable shared journal; they do not execute a new mutation.
Caller outputs remain unchanged on failure. Allocation failures may surface as
capacity or I/O at the existing shared recovery boundary.

The per-epoch book revision is the common receipt's durable revision. It is not a
native balance/item revision. Every source revision remains unchanged in EAB1.
Readers verify that the receipt revision is positive and no newer than the book;
whole-epoch audit/activation must also prove global membership and coverage.

## Verification

`python3 tests/async/test_flatfile_accounting_baseline.py` executes the production
storage and shared journal on disposable native-filesystem fixtures under ASan
and UBSan. It covers exact retry, cross-batch duplicate holdings/items, separate
epochs, corruption, unchanged synthetic native files, full-size witness retention,
allocation failures, saturated persisted indexes, orphan witnesses, disjoint merges
into an existing receipt segment, combined-bundle limits and interruptions around
initialization and batch commits.
These are storage tests, not evidence of a complete gameplay cutover rehearsal.

## Current format and registration

This increment follows PR #609 and preserves the existing v2 shared journal:
48 bytes of envelope plus a 2-byte operation count, within the existing 256 MiB
and 32-operation limits. Baseline receipts use DURECR2 and explicitly record
failure stage `none`. Lookup requires that stage, zero result code and an empty
result before accepting the retained baseline. Other valid failure stages do
not prove a successful opening.

All three baseline file classes are registered in the lifecycle manifest and
covered by the backup fixture. The existing shared transaction journal is also
registered as their recovery dependency; this adds no journal format or new
journal file. The lifecycle inventory has 34 non-database stores, with destructive
rules still disabled. Backup tests preserve the baseline files and pending
journal; they do not establish a complete semantic baseline restore.

Recovery must precede native source capture and staging. These metadata reads
can recover an existing journal, so a future lifecycle caller must not supply
native after-images prepared before that recovery. The shared commit continues
to refuse overwriting a pending journal. Neither namespace absence nor a caller's
nonzero boundary hash authorizes initialization or activation.

## Focused result

The native ASan/UBSan baseline suite passes all 41 restart boundaries (19 during
initialization, 22 during batch commit/recovery), 606 staging and 419 lookup
allocation failures, exact replay, duplicate openings across preparations, epoch
isolation, maximum witnesses and unchanged native sentinels. Coherently checksummed
non-none failure stages, baseline hardlinks and pending-journal overwrites refuse.
The existing accounting storage suite also passes its 85 fault/recovery cases.
Lifecycle (11), backup (4), contract (26), census and changed C++ formatting checks
pass. Full server build evidence is recorded with this increment's PR.

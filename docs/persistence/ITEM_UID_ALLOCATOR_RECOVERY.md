# Native flatfile item UID recovery

The native allocator is `FLATFILE_ROOT/metadata/item_uid_allocator`. Its companion
`metadata/item_uid_allocator.initialized` is protected recovery evidence. Retain
both with the custody authority; never remove the witness to bypass a refused
reservation or boot. This describes repository behavior, not a qualified
production restore procedure.

## Reservation and refusal

Under the existing cross-process allocator lock, a reservation first durably
advances the allocator and then durably publishes the witness. The caller
receives its first UID only after both writes succeed. A failed witness write
returns an I/O failure without changing the caller's output and burns the
advanced range. A retry starts beyond that range.

The version-2 witness contains an eight-byte format tag, the next UID and allocator
revision as two little-endian 64-bit integers, and a SHA-256 digest covering the
tag and both fields. It occupies 56 bytes. Every successful reservation updates
it. Reads refuse malformed evidence and any allocator whose UID or revision is
behind the witness. Missing allocator state with a witness or surviving legacy
custody also refuses; live items cannot reconstruct every previously issued ID.

A healthy allocator with no marker, or with the original eight-byte version-1
marker, preserves its high-water state and upgrades the witness before issuing
another range. The original marker supplies no numerical rollback bound. The
existing allocator encoding is unchanged.

## Recovery limits

The native and actual-server regressions restore an older, checksum-valid
allocator while retaining the newer witness. Both reservation and boot refuse
without rewriting either file; restoring the exact current allocator permits
normal boot. The sanitizer fixture also covers concurrent writers, legacy marker
upgrade, damage to both sealed bounds, and witness-write failures before and
after initialization.

This protects against a stale allocator file when newer witness evidence
survives. It cannot detect restoring the entire authority and witness together
to an older generation, or a legacy root from which all independent custody and
initialization evidence has disappeared. Captured-generation restore and rollback
qualification remains required. Older binaries that recognize only the version-1
marker refuse the larger version-2 witness; deleting or downgrading that witness
would discard the new rollback fence.

Implementation: `src/flatfile/flatfile_item_uid_allocator.c`. Native coverage:
`tests/async/flatfile_item_uid_allocator_harness.cpp` and
`tests/async/test_flatfile_item_uid_allocator.py`. Actual-server coverage:
`tests/async/test_flatfile_boot_preflight.py` with its supplied-binary option.
Both authority files remain separately required protected retained entries in
`migrations/data_lifecycle_manifest.json`.

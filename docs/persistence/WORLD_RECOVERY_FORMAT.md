# World Recovery Wire Format

Duris world-recovery generations use schema 13. The durable Redis value is independent of
compiler padding, host byte order, `time_t`, `unsigned long`, and native C/C++ struct size.
All integers are fixed-width little-endian values. Text fields are fixed-width byte arrays
that must contain a null terminator before materialization.

## Generation framing

The generation header is exactly 64 bytes. Its literal `WR12` magic is separate from the
numeric schema version:

| Offset | Bytes | Field |
| ---: | ---: | --- |
| 0 | 4 | ASCII magic `WR12` |
| 4 | 4 | Schema version, currently 13 |
| 8 | 4 | Header size, always 64 |
| 12 | 8 | Monotonic publication sequence |
| 20 | 8 | Signed Unix timestamp |
| 28 | 8 | Payload byte count |
| 36 | 4 | CRC32 of the encoded payload |
| 40 | 4 | Mobile record count |
| 44 | 4 | Object-tree record count |
| 48 | 4 | Door record count |
| 52 | 4 | Zone-timer record count |
| 56 | 1 | Complete flag: publisher writes 1; validator requires nonzero |
| 57 | 7 | Reserved zero bytes |

Each payload record starts with an eight-byte header: four-byte payload size, one-byte
record type, one-byte record version, and two reserved zero bytes. The record version is 1.
Mobile payloads contain a 286-byte base followed by the counted 60-byte affect records,
then optional transport (`TRN1`), shopkeeper (`SHP1`), and generated-NPC (`GNP1`)
extensions in that order. Object trees contain an eight-byte tree header and counted
fixed-width items; doors and zone timers have fixed-width layouts. Object UIDs and object
timers are 64-bit; VNUMs, values, counts, states, affect durations, and zone ages are 32-bit.

The codec checks record versions, types, reserved bytes, lengths, and field layouts.
Generation validation also checks the schema, header size, complete flag, sequence,
timestamp age, payload checksum, and record counts. The complete restore path additionally
validates string termination, native-width limits, world references, and object hierarchy
before materialization.

Each object-tree item has a 32-bit flags field. `authority_required` means the item had a
live SQL custody identity when captured; those flagged items must still match the exact
UID/root/parent/VNUM/room and active state before any recovery entity is materialized.
Objects created as reconstructible world population have no custody row and remain
authenticated by the generation HMAC, but are not misrepresented as SQL-owned items.
Trees whose live custody identity disagrees with their floor location are omitted, and
player corpses are left to the authoritative corpse restore path.

Floor deltas use the same schema-13 object-tree payload prefixed by `WRF5:`. The Redis hash
field UID must match the decoded root UID before the record enters recovery planning.

## Redis storage and memory bounds

A generation is not stored as one Redis value. The season-scoped generation key contains
an exact 120-byte `WRG2` manifest with version 2, total byte length, chunk count, the fixed
1 MiB chunk size, a 32-byte lowercase hexadecimal upload token, a SHA-256 payload digest,
and an HMAC-SHA256 tag bound to deployment, season, and sequence. The generation bytes are
split across at most 128 keys qualified by sequence, upload token, and zero-based chunk
index. Every manifest and chunk expires with the configured generation TTL.

The publisher writes one chunk per command on the recovery worker, then uses the writer
fence and expected current sequence to atomically publish the manifest and pointer. A
failed publisher attempts to delete only chunk keys qualified by its sequence and upload
token. After confirmed publication, it attempts to delete the previous generation's
manifest and chunks only after validating that manifest. Cleanup is best effort;
configured TTLs expire artifacts left behind when deletion fails.

Readers validate the manifest, use `STRLEN` before every `GET`, require exact expected
chunk sizes, and reject missing, malformed, oversized, or surplus-length data. `STRLEN`
and `GET` are separate commands; returned lengths are checked after receipt.

Floor records are stored in a season-scoped hash with a sorted-set UID index. The floor
worker changes each hash field and index member in the same Redis transaction. Groups
contain at most 64 mutations. The grouping code targets 1 MiB of value bytes, but this is
not a hard ceiling in the current implementation: a larger first value is allowed, and
unsigned subtraction can then admit further values into the same group. During boot,
the loader requires equal hash/index counts, accepts at most 32,768 records, and reads
64 index members followed by one `HMGET` page at a time; it never uses `HGETALL`.

Accepted recovery payload has these application-level ceilings:

- generation bytes: 128 MiB;
- floor object payload: 16 MiB;
- generation plus floor payload: 128 MiB;
- floor records: 32,768;
- individual generation chunk payload: 1 MiB.

These limits cover accepted encoded recovery payloads. Floor payload totals exclude the
five-byte `WRF5:` prefix. Total memory use also includes Redis replies, protocol and index
page overhead, decoded native records, and recovery-planning allocations. The payload
limits do not establish a measured peak-memory limit for the process.

Generation publication, floor encoding/indexing, and Redis socket work remain background
operations. Durable reads and recovery planning occur only during boot.

The manifest remains version 2; its version is independent of the world payload schema.
Current readers require schema 13 and accept generations up to 128 MiB / 128 chunks,
including current-schema generations within the former 64 MiB / 64-chunk limit. Older
world schemas are rejected regardless of size. Readers limited to 64 MiB / 64 chunks
reject larger generations, and readers requiring a different world schema reject
schema-13 generations. If rollback makes the current generation incompatible, expect
normal zone boot until a compatible reader is deployed or the generation is replaced.
Do not assume rollback restores the larger snapshot.

## Runtime and compatibility policy

Gameplay capture retains bounded native in-process snapshots because they never leave the
process. The existing publisher thread converts a completed generation to schema 13 in
place before checksumming and Redis publication. The existing floor worker converts queued
native object snapshots before issuing its Redis command. Durable decoding occurs only
during boot recovery.

The payload is intentionally a bounded fuzzy recovery snapshot. Capture start is the
durable timestamp, capture expires after five minutes, and an expired generation is never
queued for publication. Reconstructible NPC, door, and zone state may therefore rewind
within that window. NPC equipment/inventory are omitted and NPC gold is forced to zero.
The mobile record retains its zone birthplace so recovery can idempotently reconstruct
configured `G` and `E` items after every recovered NPC has materialized; player-originated
NPC inventory is never replayed from Redis. Floor items are accepted only after stable-UID
hierarchy validation and complete SQL custody reconciliation for every item marked
`authority_required`. Snapshot encoding and Redis publication stay on the workers.
Capture failures, expiry, and rejected object trees can synchronously write log files
through `logit`.

Older schemas and floor records are rejected rather than interpreted through an ABI-dependent
compatibility path. Recovery data is reconstructible and expiring: an incompatible current
generation produces a normal zone boot, and the first successful schema-13 publication
atomically replaces the generation pointer and clears prior floor deltas.

The golden-vector and round-trip contract is:

```bash
python3 tests/async/test_world_recovery_codec.py
```

## Corpse and generated item state (schema 13 / file copyover 17)

Each schema-13 item is 3,324 wire bytes. In addition to UID/tree identity, type,
values, timers, and display strings, it records action/owner text, wear flags,
extra/anti flags, weight, material, cost, trap fields, condition, craftsmanship,
z coordinate, five character bitvectors, and all fixed item affects. Recovery
metadata `flags` remains distinct from `wear_flags`; restoration never adds
`ITEM_TAKE` to an item that did not have it. Name and short-description fields
are 513 bytes each; description and action-description fields are 1,025 bytes
each. These capacities include the null terminator, allowing at most 512 bytes
of name or short-description text and 1,024 bytes of description or action text.
Capture rejects text that does not fit with its terminator rather than truncating it.
These are bounded recovery strings, not an unbounded serialization of arbitrary
object prose.

The generated-equipment audit covers the runtime overrides in `randomeq.c`,
including its fixed affects and bitvectors. Prototype-linked extra descriptions,
linked temporary object affects, event pointers, and database bookkeeping are
not newly serialized by this change. It is not a general replacement for player
item persistence. Aggregate container weights and values are restored after
linking descendants so container insertion does not double-count saved weight.

The per-record ceiling is 2 MiB, retaining the 512-item tree limit. Floor
records use the same object-payload ceiling, with the separate five-byte `WRF5:`
prefix. Generation and total floor payload budgets are listed above.

File copyover version 17 stores each ground object as a native `uint32_t` byte
length followed by the bounded native world-recovery object tree and its live
custody entries. One native `item_ownership_runtime_entry` follows for each item
marked `WORLD_RECOVERY_ITEM_AUTHORITY_REQUIRED`, in tree traversal order; no
entry is emitted for an item absent from the runtime ledger. The byte length
covers both the tree and custody entries, within the same 2 MiB ceiling.

Copyover captures the live ledger after persistence workers have quiesced and
drained. It preserves owner type, owner ID/context, logical root/parent UIDs,
item/owner revisions, vnum and active state. Logical custody topology can differ
from the physical tree (for example, items owned by a corpse); it must not be
replaced with room ownership. Recovery validates the physical tree and the
corresponding custody entries, materializes the objects, then atomically hydrates
the runtime ledger. A hydration conflict rolls back newly created objects.
This path does not call SQL room reconciliation, including in flatfile-primary
or no-MySQL builds. Redis retains its room-only capture and SQL reconciliation
rules. Copyover remains an ABI-local process handoff, unlike the portable Redis
wire format; its custody entries are not a replacement for durable persistence.
Native file copyover uses a separate carried-item layout. Redis NPC records omit
inventory and equipment. Invalid or truncated object trees or custody records
fail recovery instead of restoring a partial corpse.

The copyover writer emits version 17. The reader accepts header versions 12-17,
using version-specific mobile and descriptor widths and gated extensions; it
does not translate arbitrary older native object layouts. Version-10/11 copyover
files are rejected. Redis generations with a world schema other than 13 and
floor records with `WRF4:` or an older item layout are also rejected; the `WRF5:`
prefix alone does not make an old item layout compatible. When upgrading from
an incompatible native object layout, use a cold restart and expect normal zone
boot if Redis recovery snapshots are incompatible. Once capture and restore use
compatible layouts, subsequent copyovers and clean/crash Redis recovery retain
the current fields.

Already-damaged items are not repaired from their display names. Corpse
inspection substitutes `someone unknown` for missing or empty owner text.

The sanitizer-backed pipeline regression also compiles the file-copyover
writer/adapters, the mortal loot takeability predicate, and the numbered object
selector from production sources. It round-trips a corpse with nested generated
gloves and a non-takeable control through file copyover and Redis, then verifies
UIDs, runtime gear fields, mortal takeability, and mixed fresh/restored
`N.corpse` selection. Database services and visibility/name parsing are fixture
stubs; this is not a live server restart or an end-to-end loot transaction test.

`tests/async/test_copyover_custody.py` additionally compiles the complete production
`copyover.c` in no-MySQL mode with the real runtime ownership ledger. It calls
`copyover_save`, replaces the process with `execv`, and calls `copyover_recover`
in the new process. It verifies a ledger-backed ground corpse with a nested
container and generated gloves, including both physical and logical custody
topology. SQL reconciliation aborts if called. Idle worker drains and unrelated
player/NPC/network services are fixture boundaries; no live game or production
state is involved. The same fixture fails on the previous room-only copyover
implementation before reaching process replacement.

# World Recovery Wire Format

Duris world-recovery generations use schema 12. The durable Redis value is independent of
compiler padding, host byte order, `time_t`, `unsigned long`, and native C/C++ struct size.
All integers are fixed-width little-endian values. Text fields are fixed-width byte arrays
that must contain a null terminator before materialization.

## Generation framing

The generation header is exactly 64 bytes:

| Offset | Bytes | Field |
| ---: | ---: | --- |
| 0 | 4 | ASCII magic `WR12` |
| 4 | 4 | Schema version, currently 12 |
| 8 | 4 | Header size, always 64 |
| 12 | 8 | Monotonic publication sequence |
| 20 | 8 | Signed Unix timestamp |
| 28 | 8 | Payload byte count |
| 36 | 4 | CRC32 of the encoded payload |
| 40 | 4 | Mobile record count |
| 44 | 4 | Object-tree record count |
| 48 | 4 | Door record count |
| 52 | 4 | Zone-timer record count |
| 56 | 1 | Complete flag, always 1 for accepted generations |
| 57 | 7 | Reserved zero bytes |

Each payload record starts with an eight-byte header: four-byte payload size, one-byte
record type, one-byte record version, and two reserved zero bytes. Record version 1 has
fixed layouts for mobiles, affects, complete object trees, doors, and zone timers. Object
UIDs and timers are 64-bit; VNUMs, values, counts, states, and zone ages are 32-bit. The
codec rejects unknown schemas, record versions, types, nonzero reserved bytes, malformed
lengths, oversized records, unterminated strings, and native-width overflow.

Each object-tree item has a 32-bit flags field. `authority_required` means the item had a
live SQL custody identity when captured; those flagged items must still match the exact
UID/root/parent/VNUM/room and active state before any recovery entity is materialized.
Objects created as reconstructible world population have no custody row and remain
authenticated by the generation HMAC, but are not misrepresented as SQL-owned items.
Trees whose live custody identity disagrees with their floor location are omitted, and
player corpses are left to the authoritative corpse restore path.

Floor deltas use the same schema-12 object-tree payload prefixed by `WRF5:`. The Redis hash
field UID must match the decoded root UID before the record enters recovery planning.

## Redis storage and memory bounds

A generation is not stored as one Redis value. The season-scoped generation key contains
an exact 120-byte `WRG2` manifest with version 2, total byte length, chunk count, the fixed
1 MiB chunk size, a 32-byte lowercase hexadecimal upload token, a SHA-256 payload digest,
and an HMAC-SHA256 tag bound to deployment, season, and sequence. The generation bytes are
split across at most 64 keys qualified by sequence, upload token, and zero-based chunk
index. Every manifest and chunk expires with the configured generation TTL.

The publisher writes one chunk per command on the recovery worker, then uses the writer
fence and expected current sequence to atomically publish the manifest and pointer. A
failed publisher deletes only chunks qualified by its own upload token. Readers validate
the manifest, use `STRLEN` before every `GET`, require exact expected chunk sizes, and
reject missing, malformed, oversized, or surplus-length data.

Floor records are stored in a season-scoped hash with a sorted-set UID index. The floor
worker changes each hash field and index member in the same Redis transaction. Each
transaction group contains at most 64 mutations and 1 MiB of value bytes. During boot,
the loader requires equal hash/index counts, accepts at most 32,768 records, and reads
64 index members followed by one `HMGET` page at a time; it never uses `HGETALL`.

Accepted recovery payload has these application-level ceilings:

- generation bytes: 64 MiB;
- floor object payload: 16 MiB;
- generation plus floor payload: 64 MiB;
- floor records: 32,768;
- individual generation Redis command/reply: 1 MiB plus protocol/key overhead.

Generation publication, floor encoding/indexing, and Redis socket work remain background
operations. Durable reads and recovery planning occur only during boot.

## Runtime and compatibility policy

Gameplay capture retains bounded native in-process snapshots because they never leave the
process. The existing publisher thread converts a completed generation to schema 12 in
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
`authority_required`. No Redis, SQL,
filesystem, process, or logging I/O is added to gameplay capture.

Older schemas and floor records are rejected rather than interpreted through an ABI-dependent
compatibility path. Recovery data is reconstructible and expiring: an incompatible current
generation produces a normal zone boot, and the first schema-12 publication atomically
replaces the generation pointer and clears prior floor deltas.

The golden-vector and round-trip contract is:

```bash
python3 tests/async/test_world_recovery_codec.py
```

## Corpse and generated item state (schema 12 / file copyover 11)

Each schema-12 item is 728 wire bytes. In addition to UID/tree identity, type,
values, timers, and display strings, it records action/owner text, wear flags,
extra/anti flags, weight, material, cost, trap fields, condition, craftsmanship,
z coordinate, five character bitvectors, and all fixed item affects. Recovery
metadata `flags` remains distinct from `wear_flags`; restoration never adds
`ITEM_TAKE` to an item that did not have it. Text retains the existing fixed-width capture convention (80-byte names and
short descriptions, 160-byte room descriptions); action text has a 160-byte
field including its terminator. These are bounded recovery strings, not an
unbounded serialization of arbitrary object prose.

The generated-equipment audit covers the runtime overrides in `randomeq.c`,
including its fixed affects and bitvectors. Prototype-linked extra descriptions,
linked temporary object affects, event pointers, and database bookkeeping are
not newly serialized by this change. It is not a general replacement for player
item persistence. Aggregate container weights and values are restored after
linking descendants so container insertion does not double-count saved weight.

The per-record ceiling is 512 KiB, retaining the 512-item tree limit. Floor
records use the same increased ceiling; generation and total floor budgets
remain unchanged.

File copyover version 11 stores each ground object as a native uint32 byte
length followed by the bounded native world-recovery object tree. It uses the
same capture, semantic validation, materialization, and custody reconciliation
as Redis. File copyover remains an ABI-local process handoff format, unlike the
portable Redis wire format. Mob inventory encoding is unchanged. Invalid or
truncated object trees fail recovery instead of restoring a partial corpse.

Version-10 copyover files, schema-11 generations, and `WRF4:` floor records are
incompatible and rejected. Do not hotboot from an older executable into this
version expecting the old handoff file to load: schedule the upgrade as a cold
restart and account for the existing policy of normal zone boot when recovery
snapshots are incompatible. Once both capture and restore run this version,
subsequent copyovers and clean/crash Redis recovery retain the new fields.

Already-damaged items are not repaired from their display names. Corpse
inspection substitutes `someone unknown` for missing or empty owner text.

The sanitizer-backed pipeline regression also compiles the file-copyover
writer/adapters, the mortal loot takeability predicate, and the numbered object
selector from production sources. It round-trips a corpse with nested generated
gloves and a non-takeable control through file copyover and Redis, then verifies
UIDs, runtime gear fields, mortal takeability, and mixed fresh/restored
`N.corpse` selection. Database services and visibility/name parsing are fixture
stubs; this is not a live server restart or an end-to-end loot transaction test.

# World Recovery Wire Format

Duris world-recovery generations use schema 9. The durable Redis value is independent of
compiler padding, host byte order, `time_t`, `unsigned long`, and native C/C++ struct size.
All integers are fixed-width little-endian values. Text fields are fixed-width byte arrays
that must contain a null terminator before materialization.

## Generation framing

The generation header is exactly 64 bytes:

| Offset | Bytes | Field |
| ---: | ---: | --- |
| 0 | 4 | ASCII magic `WRS9` |
| 4 | 4 | Schema version, currently 9 |
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

Floor deltas use the same schema-9 object-tree payload prefixed by `WRF3:`. The Redis hash
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
process. The existing publisher thread converts a completed generation to schema 9 in
place before checksumming and Redis publication. The existing floor worker converts queued
native object snapshots before issuing its Redis command. Durable decoding occurs only
during boot recovery.

The payload is intentionally a bounded fuzzy recovery snapshot. Capture start is the
durable timestamp, capture expires after five minutes, and an expired generation is never
queued for publication. Reconstructible NPC, door, and zone state may therefore rewind
within that window. NPC equipment/inventory are omitted and NPC gold is forced to zero;
floor items are accepted only after stable-UID hierarchy validation and complete SQL
custody reconciliation. No Redis, SQL, filesystem, process, or logging I/O is added to
gameplay capture.

Schema 8 and `WRF2:` data are rejected rather than interpreted through an ABI-dependent
compatibility path. Recovery data is reconstructible and expiring: an incompatible current
generation produces a normal zone boot, and the first schema-9 publication atomically
replaces the generation pointer and clears prior floor deltas.

The golden-vector and round-trip contract is:

```bash
python3 tests/async/test_world_recovery_codec.py
```

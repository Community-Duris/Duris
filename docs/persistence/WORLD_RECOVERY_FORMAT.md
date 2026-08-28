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

## Runtime and compatibility policy

Gameplay capture retains bounded native in-process snapshots because they never leave the
process. The existing publisher thread converts a completed generation to schema 9 in
place before checksumming and Redis publication. The existing floor worker converts queued
native object snapshots before issuing its Redis command. Durable decoding occurs only
during boot recovery.

Schema 8 and `WRF2:` data are rejected rather than interpreted through an ABI-dependent
compatibility path. Recovery data is reconstructible and expiring: an incompatible current
generation produces a normal zone boot, and the first schema-9 publication atomically
replaces the generation pointer and clears prior floor deltas.

The golden-vector and round-trip contract is:

```bash
python3 tests/async/test_world_recovery_codec.py
```

# Portable copyover format and recovery

Copyover file version **18** replaces the native version 17 writer. New files use
magic `DCOF`, explicit little-endian integers, typed records with declared payload
lengths, and a whole-file CRC-32. Native `COPY` versions **12–17** remain readable
only on the known compatible legacy ABI. Redis recovery and its in-memory buffer
interfaces are unchanged.

The format is implemented in `src/persistence/copyover_codec.c`. Its explicit
integer encoding and checksum follow the local player save journal pattern.
[Luminari's portable-persistence conversion](https://github.com/LuminariMUD/Luminari-Source/commit/ce4fe1681a56bf509a3509281d2e120e7c761467)
is a pattern reference for bounded decoding and atomic publication; that change
converted board and house persistence, rather than copyover.

## Version 18 header

The header is exactly 64 bytes. All multibyte fields are little endian. Signed
integers use two's complement. Lengths and counts are unsigned.

| Offset | Width | Field |
| ---: | ---: | --- |
| 0 | 4 bytes | Magic `DCOF` |
| 4 | u32 | File version, exactly 18 |
| 8 | u32 | Header size, exactly 64 |
| 12 | u32 | Byte-order marker `0x01020304`, stored `04 03 02 01` |
| 16 | i64 | Timestamp, Unix seconds; must fit the recovering host's `time_t` |
| 24 | u64 | Payload bytes, exactly file size minus 64 |
| 32 | u32 | CRC-32 of bytes 0–31 followed by bytes 36 through EOF |
| 36 | u32 | Descriptor count |
| 40 | u32 | Mob count |
| 44 | u32 | Ground object tree count |
| 48 | u32 | Door count |
| 52 | 3 × i32 | Telnet, TLS, and WebSocket listener descriptors; `-1` means absent |

CRC-32 uses the reflected IEEE polynomial `0xedb88320`, initial value
`0xffffffff`, and final complement. The checksum covers the version, counts,
listener descriptors, record framing, and every payload. The checksum field
itself is excluded. This detects accidental corruption; it is not authentication.

## Record framing and field order

Every record starts with `u16 kind`, `u16 record version` (exactly 1), and `u32
payload length`, excluding the eight-byte record header. Unknown kinds, versions,
unexpected order, wrong lengths, and trailing data reject the file. Fixed strings
include a NUL within their declared width; the writer zeroes unused bytes of
descriptor, mob, and telemetry strings. There is no native alignment padding.

| Kind | Payload bytes | Fields, in encoding order |
| ---: | ---: | --- |
| 1: descriptor | 670 | i32 fd; strings player name[50], host[50], host2[254]; i8 terminal type; five i32 values (GMCP, compression, room rnum, MTTS flags, retired charset field); client[64], retired terminal[32]; i32 fighting type/id, fighting name[50]; u32 pet count; three arrays of ten i32 values (pet vnums, hit, max hit); u8 death retry pending, i32 retry delay, u64 corpse UID |
| 2: telemetry | 183 | i32 fd, player name[50], u8 validity; session producer boot/process IDs, sequence, subject ID (four u64); i32 player ID; eleven u64 values (season, environment, previous producer boot/process IDs, checkpoint revision, six cumulative counters); u32 quality flags |
| 3: mob | 356 | Twelve i32 values (vnum, instance ID, room vnum, hit/max hit, mana/max mana, vitality/max vitality, position, fighting type/id); fighting name[50]; u32 affect count; 43 i32 equipment vnums; u32 inventory count; i32 gold/birthplace; four i32 transport values (origin, destination, state, step), rider[50]; i32 shopkeeper shop ID |
| 4: affect | 59 | i16 type, i8 wear-off message index, i32 duration, u32 flags, i32 modifier, two u8 locations, u16 level, five u64 bitvectors |
| 5: carried item | 4 | i32 vnum. The legacy writer's uninitialized, unused UID is intentionally omitted. |
| 6: generated NPC | 8 + state length | Existing explicit `GNP1` extension: four-byte magic, u32 state length, bounded generated NPC identity/stats/wallet payload |
| 7: object tree | Variable | u32 portable tree byte length; existing explicit world object encoding (i32 room vnum, u32 item count, 3324 bytes per item); u32 custody count; 62 bytes per custody entry |
| 8: door | 12 | i32 room vnum, direction, door flags |

The payload contains every descriptor, then one telemetry record per descriptor,
then every mob followed immediately by its affects, inventory, and generated NPC
record. Ground object trees and doors follow. Generated NPC records are present
even when their state length is zero.

The portable object item encoding preserves every captured UID, parent/root
relationship, vnum, type, authority flag, value, timer, string, equipment flag,
material/weight/cost/trap field, bitvector, and object affect. Copyover reuses the
existing object field encoder without changing Redis's format. Its additional
custody entry encodes three u64 item/root/parent UIDs, u8 owner type, four u64
values (owner ID, context ID, item revision, owner revision), i32 vnum, and u8
custody state. Custody order must match the physical tree's authority-required
items. Captured custody roots may describe a distinct ledger domain within a
physical corpse tree; the existing handoff preserves that distinction.

Vnums, instance IDs, room indices, resource values, equipment values, affect
duration/modifiers, transport values, and shop IDs retain their signed i32
representation, including valid `-1` sentinels. Terminal and wear-off types retain
their signed byte representation. Flags, bitvectors, UIDs and revisions retain
their entire unsigned range. A recovering host rejects values its runtime types
cannot represent, instead of narrowing them. Telemetry's unknown player ID is
signed `-1`; zero remains the absent identity/counter value.

Combat links are carried by descriptor and mob records. Legacy file headers'
separate combat and zone counts were always written as zero and their sections
were never emitted or recovered; nonzero values are rejected. Redis's separate
zone-age persistence is unchanged.

## Bounds and validation

The file limit is 128 MiB, each record payload is at most 2 MiB, each object tree
has 1–512 items, and each mob has at most 32768 affects and 32768 inventory
records. Mob affects and inventory are restored in full within these limits;
the previous 64/256 recovery truncation is removed. Descriptor and listener FDs
are signed 32-bit values, independent of the connection count and the former
`select()` bitmap limit; listeners may be `-1`. Listener and player FDs cannot
collide, player FDs are positive, and duplicate player names are rejected. Each
world section has an additional ceiling of one million records, subject to the
file limit and necessary minimum byte counts.

File size is bounded before the read buffer is allocated. Header counts must fit
their runtime type and the available bytes. Record lengths, mob child counts,
tree lengths/item counts, and custody counts are checked against remaining bytes
before their allocations. The decoder validates NUL termination, death retry
state (pending delays 4–60; absent delays/UIDs zero), object parent order, globally
unique item UIDs, field ranges, and custody identity/state without gameplay calls.
It accepts a complete temporary snapshot only after every record and EOF passes.
Failure leaves the decoder's output unchanged.

Telemetry remains optional to game-state continuity. Capture or telemetry flush
failure writes absent handoffs; allocation failure while retaining decoded
telemetry consumes its bounded records and recovers sessions as absent. Invalid
optional identities are ignored, and ambiguous or unusable metadata cannot
borrow another player's session. Framing, checksum, and required-state allocation
failures reject the entire file.

## Legacy compatibility and deployment

| Native version | Descriptor bytes | Mob bytes | Additional state |
| ---: | ---: | ---: | --- |
| 12 | 660 | 288 | Object trees and live custody; no durable shopkeeper provenance |
| 13 | 660 | 356 | Transport state and durable shopkeeper provenance |
| 14 | 660 | 356 | Generated NPC extension |
| 15 | 660 | 356 | Native telemetry header/entries |
| 16 | 660 | 360 | Exact shopkeeper shop binding; `-1` for older records |
| 17 | 680 | 360 | Death retry state |

The legacy ABI gate requires little-endian LP64: 32-bit `int`, signed 64-bit
`time_t`, 64-bit `unsigned long`, and the checked historical sizes and offsets of
all nested structures. These include the 40-byte header, 64-byte affects,
16-byte carried entries, 3328-byte object items, 72-byte custody entries, and
200-byte telemetry entries. Legacy padding is ignored; the unused carried UID
is never used as authority. All supported legacy files receive complete bounds
and state validation before recovery. Unknown versions, reversed byte order,
incompatible layouts, and malformed/truncated input are refused with a reason.

Old files have no ABI tag or checksum. Compatibility can only be established for
the known layout, and corruption that still forms valid legacy values cannot be
reliably detected. Do not transfer legacy files between ABIs or treat their
padding as meaningful state. Version 18 is independent of native padding and
field widths on disk, while recovery still requires representable runtime values.

For an upgrade, a running version 17 binary can write its existing file and exec
the new reader on the compatible host. Subsequent copyovers write version 18.
An older binary cannot read version 18: rollback to an old reader requires a
normal restart from the acknowledged durable player/world authorities, or the
original compatible legacy handoff. Do not change a file's version number to
force acceptance. Socket descriptors describe this process's inherited sockets;
a portable file does not migrate live connections between machines.

## Publication and recovery failures

All existing refusal and durability prerequisites remain ahead of publication:
death recovery must be serializable; starter grants must finish; every connection
must be a preservable playing plain Telnet connection; ship/locker/maintenance,
critical commands/outbox, acknowledged terminal player saves, final player and
locker drains, Redis recovery drains, and shopkeeper snapshots must succeed.
TLS, WebSocket and non-playing connections cancel copyover while the server
remains live. MCCP ends only after publication is durable.

The writer creates `COPYOVER_STATE_FILE + ".tmp"` exclusively with mode 0600 and
refuses an existing temporary file or symlink. It writes explicit records,
validates the complete result, seals the header/CRC, flushes and fsyncs the file,
checks close, atomically renames it to the destination, and fsyncs the parent
directory. Only then can Redis release its fence and sockets change for exec.
Before rename, failure removes the writer's temporary file and preserves any
previous published file. A failed directory sync may leave the new sealed file
at the destination; copyover still aborts and resumes workers. Failed exec also
resumes the worker/Redis handoff guards. Never reuse an abandoned snapshot while
the old process has continued gameplay.

Recovery validates the whole file before allocating a descriptor, loading a
player, spawning a mob, hydrating custody, or changing a door. Runtime guards
still check inherited sockets, authoritative account/player loading, quarantine,
death retry scheduling, prototype/room availability, and object materialization
with custody conflict checks and per-tree rollback. A descriptor belongs to the
cleanup list before death retry restoration can fail.

Successful recovery removes the consumed file. Failed decoding or materialization
resets listener outputs to `-1`, leaves the file available for inspection, and
returns failure to the existing graceful cold-restart path. Required runtime
failures can occur after some valid state is materialized; that process must
follow the cold-restart path, rather than continue a partially recovered world.

After an interrupted save, inspect a stale `.tmp` and confirm no writer is using
it before moving it aside. After failed recovery, preserve the retained file and
the diagnostic reason before moving it aside and restarting normally. No test
or operator repair should use real player data as a codec fixture.

## Focused verification

`tests/async/test_portable_copyover_codec.py` compiles the production codec with
ASan/UBSan, compares complete representative bytes to an independent encoder and
a checked-in golden fixture, exercises all record types and signed/unsigned
sentinels, reads synthetic legacy versions 12–17, sweeps every truncated prefix
and a bit flip at every byte, and rejects impossible lengths/counts, unknown
versions, invalid identities, and incompatible legacy examples. It also injects
sync and allocation failures and retains records beyond the old 64/256 mob
child limits.

`test_copyover_custody.py` links the production save/exec/recovery code with a
synthetic world and real custody ledger. It covers nested corpse trees, custody
conflict rollback, write/file sync/close/rename/directory sync/exec failures,
temporary symlink refusal, unsupported transports, and decoding/materialization
failure. The telemetry, save guards, shopkeeper compatibility, generated NPC,
world recovery, singleton, player journal and existing copyover runtime journey
tests cover the surrounding persistence and gameplay contracts.

Run the focused scripts directly with Python 3, `./scripts/format.sh --check`,
and `make -C src`. The actual Telnet/MCCP journey accepts an absolute flatfile
server path and creates its own disposable player/world files. No production
database, migration, existing player account, or live server is required.

Build `bin/tests/coin-death-inspector` first with
`python3 tests/async/test_flatfile_player_repository.py --build-inspector bin/tests/coin-death-inspector`
before running the socket journey. On WSL, journal and journey temporary state
must use a native POSIX filesystem so ownership and mode checks are meaningful.

Verification uses x86-64 Linux. Independent golden-byte encoding establishes
the on-disk representation; a 32-bit or big-endian executable has not been run.

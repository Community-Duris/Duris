# World Recovery Pipeline

Optional Redis crash recovery uses a long-lived in-process publisher instead of a
forked serializer. The game thread incrementally captures one sequence-numbered
generation across NPCs, floor objects, doors, and zone timers. Each pulse is bounded by
record count and elapsed time, each record has a byte ceiling, and the complete retained
generation has a fixed memory ceiling.

The publisher receives only owned framed bytes. It cannot traverse live characters,
objects, rooms, exits, or zones. It seals the generation with schema version, timestamp,
sequence, record counts, payload length, completeness, and CRC32.

## Atomic Publication

The worker passes the immutable generation blob to one Redis Lua compare-and-set. The
script verifies the writer token and expected prior pointer while atomically writing
`mud:season:<epoch>:world_state:generation:<sequence>`, swapping the small
`mud:season:<epoch>:world_state:current` pointer and diagnostic metadata, consuming the
stable floor hash, and renewing the lease. A rejected script leaves the previous current
generation recoverable. After a verified swap, the previous blob is removed.

Boot trusts neither the diagnostic `valid` flag nor a partial key set. It loads the
current pointer and accepts the referenced generation only when magic, schema, header
size, exact sequence, age, payload length, completeness, record framing/counts, and
checksum all validate.

## Floor-Delta Boundary

Pending floor additions/removals are flushed before capture starts. While a capture,
publish, or unconsumed completion exists, newer local deltas remain in the retry buffer.
Only the exact acknowledged generation may clear the pre-capture Redis delta set. A
failed, timed-out, or stale publish leaves that set intact, while post-boundary local
deltas remain eligible for the next normal flush.

Each flush buffers all bounded hash mutations and collects their replies as one ordered
hiredis pipeline. This avoids one network round trip per delta. The in-memory batches are
cleared only after every reply succeeds, so a failed exchange remains retryable.

## Lifecycle And Health

The ordinary pulse advances capture and consumes typed completions. Copyover and
shutdown wait to bounded deadlines for capture, publication, exact acknowledgement, and
floor-boundary cleanup; failure cancels the process transition. `world persistence`
reports aggregate capture, queue, bytes, sequence, runtime, retry, and failure health
without object, room, or character identity.

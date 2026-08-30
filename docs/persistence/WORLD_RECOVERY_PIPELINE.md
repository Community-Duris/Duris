# World Recovery Pipeline

Optional Redis restart and crash recovery uses a long-lived in-process publisher instead of a
forked serializer. The game thread incrementally captures one sequence-numbered
generation across NPCs, floor objects, doors, and zone timers. Each pulse is bounded by
record count and elapsed time, each record has a byte ceiling, and the complete retained
generation has a fixed memory ceiling.

The generation is an explicitly fuzzy recovery snapshot, not a point-in-time transaction.
Its timestamp is capture start, so age is conservative relative to every record. Capture
may span at most five minutes; an expired capture is discarded before publication, its
failure completion resumes the floor worker, and a later periodic request retries from a
new sequence. The game thread still performs at most 64 capture steps or 2 ms of capture
work per pulse. Door capture scans all fixed directions for one room per step, avoiding
one budget step per absent exit.

Fuzzy state is restricted to reconstructible NPC position/state, doors, zone timers, and
world-pop objects. NPC equipment and inventory are omitted, and NPC-carried gold is
captured as zero so a cross-time generation cannot replay currency into the persisted
player economy. Floor item trees retain stable UIDs and hierarchy. Capture marks items
that have live SQL custody and omits trees whose custody disagrees with their floor
location; restore requires complete SQL reconciliation of every marked item before
materializing anything. Reconstructible world-pop objects stay HMAC-authenticated without
inventing SQL custody, and player corpses remain with the separate authoritative corpse
restore path. Player and ship state remain SQL-authoritative.

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

Pending floor additions/removals are submitted to a bounded background worker. Before
capture, an ordered barrier confirms all earlier mutations and pauses publication of
later mutations. Only the exact acknowledged generation may atomically clear the stable
pre-capture hash. Completion or capture failure resumes post-barrier work. Each immutable
batch remains a hiredis pipeline, avoiding one network round trip per delta without
blocking the game loop.

## Lifecycle And Health

The ordinary pulse advances capture and consumes typed completions. Copyover and
shutdown wait to bounded deadlines for capture, publication, exact acknowledgement, and
floor-boundary cleanup; failure cancels the process transition. `world persistence`
reports aggregate capture, queue, bytes, sequence, active/last capture age, expiry,
runtime, retry, and publication-failure health without object, room, or character identity.

After a successful graceful drain, the fenced writer records an expiring marker for the
exact current sequence. Boot consumes that marker once and labels a matching valid
generation as clean-restart recovery. A missing or mismatched marker is crash recovery.
Successful materialization consumes only that exact generation and leaves the publisher
enabled for the new process lifetime.

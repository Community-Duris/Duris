# World Recovery Pipeline

Optional Redis restart and crash recovery uses a long-lived in-process publisher instead of a
forked serializer. The game thread incrementally captures one sequence-numbered
generation across NPCs, floor objects, doors, and zone timers. Each pulse has a capture-step
limit and a cooperative elapsed-time budget; each record has a byte ceiling, and the complete
retained generation has a fixed 128 MiB memory ceiling. The existing 2 MiB per-record and 512-item
tree ceilings remain unchanged.

The generation is an explicitly fuzzy recovery snapshot, not a point-in-time transaction.
Its timestamp is capture start, so age is conservative relative to every record. Capture
may span at most five minutes; an expired capture is discarded before publication, its
failure completion resumes the floor worker, and a later periodic request retries from a
new sequence. The game thread attempts at most 1,024 capture steps per pulse and checks a
two-millisecond deadline between steps. A step already in progress is allowed to complete.
Door capture skips absent and non-door directions within a step, emits at most one door
record per step, and advances to the next room after exhausting the current room's directions.

Fuzzy state is restricted to reconstructible NPC position/state, doors, zone timers, and
world-pop objects. NPC equipment and inventory are omitted from the snapshot payload,
and NPC-carried gold is captured as zero so a cross-time generation cannot replay currency
into the persisted player economy. Restore reconstructs applicable zone-defined NPC items
under population, artifact, and duplicate checks. Floor item trees retain stable UIDs and
hierarchy. Capture marks items that have live SQL custody and omits trees whose custody
disagrees with their floor location; restore requires complete SQL reconciliation of every
marked item before materializing anything. Reconstructible world-pop objects stay
HMAC-authenticated without inventing SQL custody, and player corpses remain with the
separate authoritative corpse restore path.

Player and ship state remain authoritative in the native backend selected by
`PERSISTENCE_MODE`: SQL in `mariadb-primary`, or private flat-file authority in a
client-free `flatfile-primary` build. Redis recovery does not replace that authority.
The SQL reconciliation requirement for marked recovery items still applies in this
checkout; the client-free adapter refuses a nonempty marked set.

The publisher receives only owned framed bytes. It cannot traverse live characters,
objects, rooms, exits, or zones. It seals the generation with schema version, timestamp,
sequence, record counts, payload length, completeness, and CRC32.

## Atomic Publication

The worker stages the immutable generation in chunks of at most 1 MiB before publication.
Keys use the configured `REDIS_NAMESPACE`, validated as `duris:<ENVIRONMENT>:<deployment>`,
followed by `:season:<epoch>:world_state:...`. One Redis Lua compare-and-set verifies the
writer token and expected prior pointer, then atomically writes the 120-byte authenticated
manifest at `<namespace>:season:<epoch>:world_state:generation:<sequence>`, swaps the
`<namespace>:season:<epoch>:world_state:current` pointer and diagnostic metadata, clears
the stable floor hash and index, and renews the writer lease. A compare-and-set rejection
leaves the previous current generation selected. After a verified swap, the worker
attempts to remove the previous manifest and chunks; their TTLs bound any leftovers.

Boot authenticates the manifest before allocating the generation buffer, reads chunks
at their exact expected lengths, and verifies the complete generation's SHA-256 digest.
It also requires the existing generation validation: magic, schema, header size, exact
sequence, age, payload length, completeness, record framing/counts, and CRC32. Diagnostic
metadata and a partial key set do not establish a recoverable generation.

## Floor-Delta Boundary

Pending floor additions/removals are submitted to a bounded background worker. Before
capture, an ordered barrier confirms all earlier mutations and pauses publication of
later mutations. The fenced generation's Lua publication commit atomically clears the
stable pre-capture floor hash and index. The game thread learns the outcome through a
typed completion; publication can commit even if the reply is lost. Successful and failed
completions, including capture failure, resume post-barrier work. Each immutable batch
remains a hiredis pipeline, avoiding one network round trip per delta without blocking
the game loop.

## Lifecycle And Health

The ordinary pulse advances capture and consumes typed completions. Copyover and ordinary
shutdown use cooperative deadlines to drain outstanding recovery work. A drain timeout
cancels those guarded transitions. A completed drain does not prove that the last
generation published successfully: terminal capture or publication failures can finish
with no new acknowledged sequence. Copyover also flushes and drains floor work and
releases the writer fence before exec. Final teardown logs drain failures and may cancel
workers.

`world persistence` reports aggregate capture, queue, bytes, submitted/acknowledged
sequence, capture age, worker runtime, and publication-failure health. `redis detailed`
additionally reports capture expirations and the last capture duration. Publication
and writer retry attempts are reported in logs. These aggregate health rows contain
no object, room, or character identity.

For eligible shutdowns with drained world and floor work, the fenced writer attempts to
record an expiring marker for the exact current sequence. Marker failure is logged without
cancelling shutdown. Boot consumes that marker once and labels a matching valid generation
as clean-restart recovery. A missing or mismatched marker is crash recovery.

After successful materialization, boot attempts to consume that exact generation under
the writer fence; failure is logged while boot continues. Boot then re-enables periodic
publication for the new process lifetime.

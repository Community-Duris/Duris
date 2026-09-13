# Shop and flight singleton recovery

Issue #235 reported five copies of flight dragon 47030 and sign 420 at room
543662, and six copies of keeper 29429 at shop room 29437. Startup created
transports and restored saved keepers before world recovery loaded more copies.

Transport initialization now runs after file copyover, Redis clean/crash
recovery, or the Redis normal-boot fallback. It reuses the service's dragon
anywhere in the world and keeps one sign at each configured origin. Repeated
calls do not create extra dragons, signs, or movement events. Only the explicit
transport catalog and signs at its origins are reconciled.

New snapshots retain a transport's origin, destination, movement state, path
step, and passenger name. Recovery validates the saved position against the
recomputed route. A co-located recovered passenger resumes the flight. An
unoccupied dragon returns along its route. Missing, obsolete, or invalid route
state returns the dragon to its configured origin. When old duplicates include
multiple occupied dragons, one flight survives and extra passengers are landed
at that service's origin before their extra dragon is removed. Carried and worn
items on retired dragons are transferred to the survivor.

File copyover version 13 reads version 12 records as well. Version 12 did not
record routes or riders, so those old flights cannot be resumed from their
snapshot; their dragons are returned home. Redis schema 12 accepts its original
NPC record and a bounded optional `TRN1` transport tail. Ordinary NPC wire
records are unchanged. An older binary cannot read the new transport tail and
will reject that Redis generation rather than interpret it incorrectly.

Shop inventory has different authorities in each recovery mode:

- SQL file copyover restores live keepers and stock from the copyover file;
  startup skips the older SQL keeper snapshot.
- Redis restores NPC location/state without inventory. Its reconciliation
  retains the keeper already restored from the durable shop store and removes
  recovered duplicates and their regenerated template stock.
- Flat-file trade records remain authoritative in both modes, including item
  custody and shop revisions.
- Without a durable incumbent, reconciliation selects the recovered keeper
  with the most stock/equipment, breaking ties by instance ID. File-recovered
  duplicates contribute their non-produced stock and equipment to the survivor.
  Produced stock is replenished for the selected shop.

Shop binding uses the configured room, birthplace, or an unambiguous roaming
shop. Two shops may share a mobile vnum. Flat-file restoration replaces matching
mobile/room incumbents, and shop commands use the same room-aware binding.
Unbound ordinary NPCs and objects are outside this cleanup.

`python3 tests/async/test_world_singletons.py` compiles production reconciliation,
movement, and NPC capture/restore code with isolated world I/O under ASan/UBSan.
It covers the reported records, duplicate snapshots, five stock recovery cycles,
five passenger recovery cycles through the wire codec, completion and return of
the flight, repeated initialization, durable stock/equipment, shared vnums, and
legacy/occupied duplicates. Existing shopkeeper population tests exercise normal
and forced resets. Existing recovery pipeline tests exercise failed Redis
materialization and fallback. These are isolated regressions, not a production
recovery experiment.

The next full-world boot or copyover on the updated binary applies reconciliation
and logs the number of extra keepers and dragons removed. Merging the change
does not restart a running server or directly alter production state.

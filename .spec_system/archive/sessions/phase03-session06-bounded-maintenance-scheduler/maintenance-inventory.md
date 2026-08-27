# Recurring Maintenance Inventory

| Activity | Previous cadence | Classification | Session 06 disposition |
|---|---:|---|---|
| Auction due scan | 60s | Database scan plus typed settlement submission | Bounded `id` cursor worker job; durable completion publishes settlement commands |
| Poll expiration | 300s | Database scan/mutation | Bounded `id` cursor worker job with idempotent update |
| Epic task catalog | 60m refresh | Database scan plus immutable publication | Bounded `number` cursor worker job; atomic game-thread catalog publication |
| Epic-zone balance | 120s | Database scan/mutation | Bounded epic-zone primary-key cursor and exact conditional updates |
| Level cap | 60s check | Database aggregate/mutation | Transactional worker job with operation marker and bounded result notification |
| Zone trophy reduction | 60s check, property-driven execution | Database scan/mutation | Composite `(pid, zone_number)` cursor with per-chunk marker |
| Epic-zone modifiers | 60s check, property-driven execution | Database scan/mutation | Primary-key cursor with per-chunk marker and timer publication |
| Boon maintenance | 60s | Mixed live state plus database scan/mutation | Bounded primitive snapshot, `id` cursor worker mutation, durable pure completion |
| Web status | 75s | Small serialization plus filesystem replacement | Bounded immutable game-thread capture and atomic worker replacement |
| Cargo market | 60s check, two property-driven timers | Mixed live calculation plus database/file-free persistence | Retry-stable primitive snapshot and transactional worker persistence; independent timers preserved |
| Operational statistics | 75s | Descriptor aggregation plus DB/file output | Bounded primitive aggregation; idempotent worker DB insert and append |
| Ship activity | 1s | Pure live-world simulation | Remains game-thread work with deterministic instance offset |
| Ferry activity | 1s | Pure live-world simulation | Remains game-thread work with deterministic instance offset |
| Random map spawn | 120s | Pure live-world simulation | Remains game-thread work with deterministic instance offset |
| Short affects | Existing `SHORT_AFFECT` | Pure live-world simulation | Remains game-thread work with deterministic instance offset |
| Newcomer approval | 300s | Pure live-world simulation | Remains game-thread work with deterministic instance offset |
| Auction info backfill | Former hidden recurring compatibility repair | Migration/backfill, not gameplay maintenance | Disconnected from recurring gameplay; retained only as migration-tooling code for Session 11 disposition |

Boot loaders, Redis recovery, connection housekeeping, combat, event execution, and
command/prompt processing retain their existing lifecycle ownership and are not
maintenance-registry jobs.

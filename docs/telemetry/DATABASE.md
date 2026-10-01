# Telemetry SQL schema and role contract (#261)

Migration `0014_telemetry_storage` adds exactly six InnoDB tables. The additive
migration `0017_telemetry_rollup_support` adds the two replay-safe rollup support
stores described below. Migration `0030_telemetry_quarantine` adds a durable,
operator-controlled quarantine for record-specific SQL failures. Existing
immutable migrations retain their original content and checksums; the current
runtime inventory is 203 tables.

| Table | Grain and ownership |
| --- | --- |
| `telemetry_interval` | Immutable tagged facts of all five record kinds; writer inserts and reads replay evidence. `ingest_id` is the keyset cursor. Global unique `(boot_id,process_id,record_seq)` also covers process-wide gaps. |
| `telemetry_session` | Latest absolute checkpoint totals plus observed enter/exit flags and quality. Scoped primary key includes environment/season and original session identity; a second global session identity unique key prevents a changed scope from creating a second projection. Writer owns insertion/update. |
| `telemetry_config` | Immutable `(environment_id,config_id)` and the complete typed effective snapshot, including its SHA-256 fingerprint and publication metadata. Writer owns insertion; publication reuse must match semantic content, excluding process-local revision and effective time. |
| `telemetry_player_day` | Rollup definition/generation/environment/season/UTC-day/subject/session contribution. The six duration counters, attributable coverage and watermark remain separate from raw session totals. |
| `telemetry_cohort_day` | Rollup definition/generation/environment/season/day/level band/primary class/race/faction/zone/config/category sums and counts. |
| `telemetry_rollup_state` | Definition/generation/environment/season committed input watermark, publication state, occurrence coverage and rebuild range. |
| `telemetry_rollup_session` | Definition/generation/environment/season/original session identity projection with greatest checkpoint revision, absolute checkpoint totals, sealed interval coverage, attribution and provisional quality. Primary key is the replay identity; `idx_rollup_session_subject` supports bounded subject keysets. |
| `telemetry_cohort_member` | Definition/generation/environment/season/UTC-day/captured cohort dimensions/category/member contribution. `membership_kind` distinguishes one subject member (zero session identity) from one original session member; the full member identity is the replay-safe primary key. |
| `telemetry_quarantine` | Durable isolation of a record whose SQL failure is classified as record-specific. The writer stores replay identity, a payload digest, the fixed-size record and payload-free failure metadata; only an explicit reviewed recovery action may change `recovery_state`. |

The tagged fact stream stores named columns, with SQL NULL for fields absent from
the selected kind. Allowed all-zero session and connection references remain zero
in their present fields. A process-wide gap therefore has zero session/scope
values; configuration carries its own environment/season and no session columns.
`schema_version` is shared between the record header and configuration snapshot,
which representation validation requires to agree. `checkpoint_revision` and
`config_revision` distinguish the two revision meanings. `gap_reason` selects the
gap enum; `lifecycle` selects the lifecycle enum. All other scalar names follow
`telemetry_types.h`. Primary fact and configuration rows have no native-layout
blob, arbitrary JSON or payload hash substituting for exact comparison of typed
content. The protected quarantine is the deliberate exception: it retains the
exact bounded record beside a semantic digest for controlled investigation and
replay. The configuration fingerprint is exactly 32 raw bytes. Named reserved
fields are validated as zero and omitted from primary facts.

UTC labels use signed BIGINT epoch microseconds, including the explicit unknown
sentinel. Monotonic endpoints, elapsed values, identities, sequences and revisions
use unsigned BIGINT. PID and zone retain signed INT semantics; bounded dimensions
retain their public integer widths. Ingestion time is separate from occurrence
labels; the repository assigns it when committing the batch.

The nonunique checkpoint index includes complete session scope and revision so a
prior observation at that revision can be compared without scanning all history.
Identical totals may occur under another replay key. Raw facts have a subject
index for later bounded subject processing. Aggregate primary keys include all
specified definition, generation, scope and attribution dimensions. No gameplay
foreign keys, triggers, scheduler, partitioning or retention procedures are added.

Rollup publication status is 0 building, 1 published, 2 superseded, 3 failed.
Versioned rollup code owns this vocabulary and must publish/supersede generations
atomically. Provisional defaults to 1; nullable occurrence bounds mean unknown.
Count columns represent the declared daily grain and cannot be summed to infer
multi-day distinct subjects or accounts. `observed_intervals` counts duration
facts only; lifecycle/gap/config/checkpoint rows contribute no interval duration.
Future rollup implementation and report activation remain separate issues.

## Roles and activation

The migration creates storage only. It never creates accounts or changes grants.
Operators provision a dedicated writer identity for the verified telemetry
connection before activation, using the same database endpoint/TLS/session policy
as the authority connection. No gameplay credentials or gameplay connection pool
may serve as a telemetry fallback.

| Role | Allowed table operations |
| --- | --- |
| Telemetry writer | SELECT and INSERT on `telemetry_interval`, `telemetry_config` and `telemetry_quarantine`; SELECT, INSERT and UPDATE on `telemetry_session`. No quarantine UPDATE/DELETE, aggregate writes or gameplay-table privileges. |
| External rollup | Bounded SELECT on `telemetry_interval`, `telemetry_config` and `telemetry_session`; SELECT, INSERT and UPDATE on `telemetry_player_day`, `telemetry_cohort_day`, `telemetry_rollup_session`, `telemetry_cohort_member` and `telemetry_rollup_state`. No gameplay writes or raw UPDATE/DELETE. |
| Reports | SELECT only on reviewed aggregate/state tables or restricted views; no unrestricted raw history or gameplay access. |
| Migration/lifecycle operator | Existing reviewed administrative workflow; distinct from runtime identities. No automatic purge is authorized. |

On a shared server permit one bounded external rollup/report operation at a time;
row/byte/runtime budgets and production load qualification remain #268/#272 work.
An auto-increment cursor is a stable committed prefix only under the single
sequential writer with ambiguous commits resolved before later batches.

## Lifecycle and recovery

All nine stores are registered in `migrations/data_lifecycle_manifest.json` with
season and terminal action `retain`, pending retention/archive/controller/export
decisions, and destructive rules disabled. The fact/config/state stores protect
replay and rebuild evidence. Telemetry is observational and never an economic or
player-save authority; its replay protection does not authorize indefinite
retention as a controller policy.

Scoped subject/PID facts and per-subject contributions require a reviewed subject
processing route before activation. No account linkage is claimed or derived from
current player rows. Aggregate disclosure also remains pending. Backup and restore
must preserve the nine tables together, including facts, quarantine evidence, config identities,
projection revisions, rollup generations and cursor state. Rebuild is possible only
while required fact detail is retained. Restored/test worlds require a fresh
environment and producer incarnation before new observations. Copyover retains the
original session identity and changes current producer/connection identities; SQL
never synthesizes elapsed handoff time. These are explicit operational gates, not
an implemented purge/export scheduler.

## Repository behavior and F/D handoff

`telemetry_repository.c` implements the unchanged public #260 API. F must register
it in the build and call init/apply from its sole joined worker; no source here
activates telemetry or adds a thread. Flat-file authority, including runtime
flat-file mode in the SQL build, returns `flatfile_disabled` without connecting.

The private SQL factory requires `TELEMETRY_DB_USER` and `TELEMETRY_DB_PASSWD` in
addition to the existing verified database configuration. Neither falls back to
`DB_USER`/`DB_PASSWD`. Target resolution, allowlisting, local/production validation,
remote TLS identity checks, UTF-8, UTC, strict SQL mode and READ COMMITTED are shared
with the authority factory. Telemetry uses a two-second connect/read/write option
and two-second InnoDB lock wait. These are connector options, not a proven end-to-end
shutdown deadline. Budget one additional ingest connection; external reports and
rollups share at most one additional separately budgeted connection.

The private writer uses MariaDB's socket accessor or Oracle MySQL's public
`MYSQL::net.fd`, matching the selected client library. It preserves descriptor
flags and adds `FD_CLOEXEC` before
the factory returns it. Failure to read or set those flags closes and refuses the
connection. Successful exec closes the old writer socket and releases its advisory
lock; failed exec leaves the original connection usable. Player sockets and the
gameplay SQL connection retain their existing ownership. For an initial upgrade
from a binary with an active inheritable telemetry socket, use an ordinary process
restart: the new factory cannot retroactively mark the old process's open socket.

The private handle obtains a nonblocking, database-scoped advisory ingest lock.
Another writer cannot claim readiness while that handle retains the lock. Losing
the connection requires reacquiring ownership before retry. This also prevents a
replacement writer from advancing the ingest cursor while an old connection is
still committing. Startup probes only the four ingest-owned tables, consistent
with the documented writer privileges. The full runtime schema validator owns
compatibility for all nine stores. No main handle, pool acquisition or fallback
path exists.

Each bounded batch uses one transaction and each candidate gets a savepoint.
Representation-invalid rows, missing configuration, scope changes and replay
conflicts are returned individually while valid neighbors can commit. A SQL error
classified as record-specific is rolled back to the record savepoint, inserted in
`telemetry_quarantine` in the same outer transaction, and represented as an
explicit coverage gap while later records continue. Connection and transaction
errors roll back/discard the complete transaction and close the private handle.
A failed COMMIT acknowledgement is explicitly ambiguous. Only committed results
contribute durable applied/duplicate/stale/rejected/quarantined health counts.

Exact replay compares the active named typed fields, including publication
metadata; native padding and inactive union bytes are ignored. Incoming
configuration materialization recomputes the contract SHA-256. Referencing facts
must match the configuration's captured environment, season and classifier/policy
versions. Configuration facts and their materialization share a transaction.

Shared `(environment_id,config_id)` reuse compares semantic content and preserves
the first materialized snapshot. A new producer may publish that same content
with a different local revision or effective time; each publication remains a
separate fact with its original metadata. Changed metadata under the same fact
key still conflicts, as does changed semantic content under a shared config ID.

Session subject/PID/environment/season are immutable across copyover. Checkpoints
are absolute counters, never additive. Newer revisions cannot decrease any bucket;
any previously observed revision with different totals conflicts, including one
older than the current projection. Accepted old observations remain immutable
facts without replacing newer totals. Missing entry/exit observations remain
incomplete; intervals never fabricate missing cumulative totals or context.

After a retryable or ambiguous failure, the repository retains a fixed-size owned
copy of the original batch and compares its named fields on retry. D must retry exactly that batch (same order,
keys and named values); a different batch is rejected before SQL. Pending identity
state is process-local, not a durable spool. Its loss on shutdown leaves the stream
incomplete rather than fencing gameplay. Cached health uses a short lock never
held across SQL. Stop requests are atomic and prevent new apply calls; F/D should
request repository stop **after** any chosen transport drain. Shutdown closes the
handle only after the worker has been joined, and is legal off that worker. Client
thread setup/cleanup is paired within each SQL call through the existing worker
initialization helper. Initialization and apply are single-worker-only APIs.

## Local validation and limitations

The new migration verifier enforces explicit local/production scope and verified
remote TLS; no remote connection falls back to plaintext or preferred-mode TLS.

The additive migration is registered in the immutable manifest and the
lifecycle/runtime inventories; no new shared runner or Makefile target is required.
Generic Python test discovery runs offline checks; SQL-only checks skip or compile
without a disposable-fixture opt-in. F owns server build integration and L owns
later integration-test registration. From a Linux checkout with compiler/database
development dependencies:

```sh
python3 tests/async/test_telemetry_contract_headers.py
python3 tests/async/test_telemetry_contract_fixtures.py
python3 tests/async/test_telemetry_repository.py
python3 tests/async/test_telemetry_schema.py
python3 tests/async/test_telemetry_migration_tls.py
python3 scripts/validate_data_lifecycle.py
python3 scripts/validate_runtime_compatibility.py
make -C src -j4
make -C src -j4 PERSISTENCE_BACKEND=flatfile
```

For a caller-provisioned **disposable loopback** MariaDB fixture with the test
schema `duris_telemetry_test`, the explicitly guarded repository suite resets that
schema's telemetry tables and consumes all ten shared golden fixtures through the
actual repository API. It also tests mixed rejections, field/padding replay,
checkpoint history, restart configuration reuse and immutable publication replay,
configuration/scope validation, immutable batch retries, five
transaction fault modes, ownership lock contention, startup recovery, disabled
behavior, stop requests and concurrent cached health:

```sh
TELEMETRY_REPOSITORY_DISPOSABLE=1 python3 tests/async/test_telemetry_repository.py --sql-fixture
# MYSQL_CONFIG may select a separate Oracle or MariaDB client installation.
TELEMETRY_REPOSITORY_DISPOSABLE=1 python3 tests/async/test_telemetry_connection.py
```

The service-free `test_telemetry_cloexec.py` covers descriptor flag preservation
and fail-closed setup. The connection test executes the actual shared factory and
its real session initialization, with a link-time credential-selection spy that checks the selected
role before using the fixture's existing passwordless root account. It does not
create users or change grants. It also holds a real advisory lock across a failed
exec, then checks socket closure and lock reacquisition after a successful exec.
This is factory-level process coverage. Full player lifecycle coverage lives in
`tests/async/run_telemetry_player_journey.py`, integrated by #384. Neither the
factory test nor its credential spy proves provisioned ingest-role permissions
or remote TLS operation. The repository fault tests similarly use
controlled connector failures; real network teardown and representative load
qualification remain L/#272. No live game, production migration, load generation,
or production activation was performed.

Schema tests additionally accept explicitly opted-in loopback `*_test` databases:

```sh
ENVIRONMENT=test TELEMETRY_SCHEMA_TEST=1 DB_HOST=127.0.0.1 DB_PORT=3306 DB_USER=root \
  DB_NAME=duris_telemetry_schema_test python3 tests/async/test_telemetry_schema_mysql.py
```

The historical MariaDB 10.11 and MySQL 8.4 verification covers the pre-0017
six-store schema, including reapply, signed/unsigned extremes, identity/index
constraints, deliberate schema damage, full runtime compatibility, and rejection of
each missing telemetry store. On Windows CRLF checkouts, checksum-sensitive
verification used an isolated LF copy; no historical migration bytes/checksums
were changed. The 0017 extension is statically verified here; parent-owned MariaDB
migration, grant, and drift checks remain required. These results do not qualify
rollup/report plans or the future load gate. Permission grant provisioning, scoped
disclosure and lifecycle activation remain explicit operator/integration work.
The runtime contract is pinned to migration head 0017. A previously built binary
pinned to head 0016 will refuse the upgraded schema; additive tables do not make a
binary-only rollback compatible. Recovery requires a reviewed compatible
binary/schema pair. Disabling telemetry retains its tables and does not authorize a
destructive down migration.

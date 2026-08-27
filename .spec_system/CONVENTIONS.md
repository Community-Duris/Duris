# CONVENTIONS.md

## Scope and Precedence

- Follow the repository `AGENTS.md`, then the conventions in this file.
- Keep changes narrow and preserve the style of nearby legacy code.
- Treat `.c` files under `src/` as C++20 sources compiled with `g++`.
- Preserve unrelated worktree changes and never expose local credentials or player data.

## Guiding Principles

- The game thread is the sole owner of mutable `P_char` and `P_obj` state.
- Normal mutation paths and per-pulse callbacks perform no external I/O.
- Workers receive immutable typed values, never live game-object pointers.
- Persist exact revisions and clear dirty state only after the matching acknowledgement.
- Give every non-idempotent economy, ownership, reward, and audit action a unique ID.
- Bound time, bytes, retries, and shutdown behavior at every external boundary.
- Preserve proven InnoDB, transaction, dedupe, pool, and locker-worker foundations.
- Base schema and performance changes on measured repository or development-clone evidence.

## Naming

- Match nearby names and established domain terms such as PID, epic, frag, rent, and locker.
- Keep existing public type names such as `P_char` and `P_obj` stable unless a scoped refactor requires otherwise.
- New functions should describe one action; state and revision fields should identify their domain.
- Boolean and result names should make success, retryability, dirty state, and acknowledgement explicit.
- Stable operation, event, revision, and query-site IDs must be unambiguous in code and telemetry.

## Files and Structure

- `src/` owns server implementation and its `Makefile`.
- `tests/async/` owns focused regression, runtime, source-contract, and DB-backed tests.
- `migrations/` is the authoritative location for schema changes and schema verification.
- `scripts/` owns startup, formatting, backup, debugging, and maintenance helpers.
- `docs/` owns architecture, database, configuration, testing, and operational guidance.
- `bin/` contains all compiled artifacts and must not be committed.
- Put changes in the existing domain module unless a new boundary is required by the persistence design.

## C++20 and Legacy Code

- Compile touched server code as C++20; do not rely on C-only compilation behavior.
- Follow `.clang-format` for touched C/C++ lines and avoid formatting unrelated legacy code.
- Keep functions focused, but do not broaden a persistence fix into unrelated modernization.
- Check all allocation, conversion, query, and serialization bounds using existing safe helpers where available.
- Initialize every output on every success and failure path before it can influence gameplay.
- Avoid mutating equipment or affects solely to build a persistence snapshot.

## Concurrency and Ownership

- Build immutable snapshot DTOs on the game thread before enqueueing worker work.
- Key ordered work by player, account, item, or other owning entity; parallelize only across independent keys.
- A worker transaction may apply revision N only when the stored revision is older than N.
- Apply worker completions on the game thread and ignore stale acknowledgements safely.
- Do not call `fork()` from the running multithreaded server for player or world persistence.
- Make worker lifecycle states, operation deadlines, retry timing, pool healing, drain, and termination explicit.
- Never hold a game lock or queue mutex across database, Redis, filesystem, or allocator-heavy work.

## Error Handling and Recovery

- Classify external failures as retryable, terminal, ambiguous commit, or overload outcomes.
- Retain dirty revisions and retry with bounded exponential backoff after retryable failure.
- A failed terminal save must leave the live character and inventory intact and retryable.
- Do not convert Redis, queue, or journal failure into a synchronous full save on the game thread.
- Alerts and counters must report actual durable success, failure, retry, spill, and recovery state.
- Recovery records require schema versions, checksums, unique IDs, atomic checkpoints, and idempotent replay.
- Shutdown and copyover stop new mutations, drain to a documented deadline, spill remaining work, then exit.

## Database Layer

### Connections

- Read `DB_HOST`, optional `DB_PORT`, `DB_USER`, `DB_PASSWD`, and `DB_NAME` from the environment; never hardcode or log their values.
- MySQL or MariaDB with InnoDB is the durable system of record.
- Set and verify `utf8mb4`, time zone, isolation level, SQL mode, and bounded connect/read/write deadlines on every connection.
- Require native TLS or a protected local socket/tunnel for non-local database hosts.
- Heal failed pool slots asynchronously and expose acquisition, execution, retry, and oldest-work age.

### Migrations

- Put additive schema changes and their verification under `migrations/`.
- Keep migrations guarded and re-runnable where practical; never alter production or run operational scripts against it.
- Use `bootstrap_multithread_safe.sql` only for an empty database.
- Apply `migrations/run_migration.sh` only to a backed-up development clone before promotion.
- Give new migrations immutable IDs and checksums and record success only after the operation completes.
- Validate required schema and connection invariants before boot writes lookup data.

### Schema and Models

- Persist monotonic revisions on parent rows for revisioned snapshot domains.
- Keep current balance or ownership rows reconcilable with immutable ledger or audit rows.
- Use foreign keys and uniqueness constraints to enforce dedupe and ownership contracts where applicable.
- Define explicit limits and errors instead of silently truncating player, inventory, or pet data.
- Add retention or archival only with documented season, reconciliation, and audit ownership.

### Queries

- Prefer typed repositories and prepared statements for new hot or sensitive paths.
- Never concatenate untrusted values into SQL; use the database client's parameter or escaping contract consistently.
- Keep hot-path lookups in memory when the source defines game memory as authoritative.
- Batch related rows and avoid N+1 queries, repeated linear scans, and per-recipient query fan-out.
- Keep predicates sargable and validate candidate indexes with representative clone data and `EXPLAIN ANALYZE`.
- Log stable site ID, error code, duration, and operation ID, never raw SQL or bound private values.

### Transactions and Idempotency

- Commit critical economy and ownership changes before reporting final gameplay success.
- Put balance and ledger, current owner and audit outbox, or equivalent coupled state in one transaction.
- Use mandatory unique command or event IDs for ambiguous commits and replay.
- Store outbox or inbox delivery state in the same transaction as the domain mutation.
- Return authoritative committed values and publish them to all relevant in-memory owners.

## Redis and Caching

- Use Redis only for reconstructible cross-process or report cache data.
- Keep player dirty state in game memory plus the durable journal, not Redis membership alone.
- Give Redis connect and command operations bounded timeouts and guard every context before use.
- Use namespaced and versioned keys, finite TTL with jitter, explicit invalidation, and single-flight rebuilds.
- Monitor cache, dirty-state, and world-recovery domains independently.
- Never put a Redis round trip in regeneration, XP, or another per-pulse hot path.

## Queues and Journals

- Use typed bounded queues with byte limits, age limits, high/low watermarks, and explicit overload behavior.
- Preallocate compact queue storage away from the hot producer path and batch compatible operations.
- Spill durable idempotent records before memory limits; unrestricted raw SQL is not a durable message format.
- Acknowledge only after destination commit and checkpoint replay only after the acknowledgement is durable.
- Track pending, oldest age, high-water mark, retries, drops, spill bytes, replay duplicates, and end-to-end latency.

## Testing

- Add or update a focused regression in `tests/async/` whenever behavior changes.
- Run the smallest relevant `python3 tests/async/test_<feature>.py` or `run_<feature>.sh` wrapper first.
- Run `make -C src` after every C/C++ change; the executable is `bin/server/dms_new`.
- Use isolated MySQL or a backed-up development clone for schema and persistence integration tests.
- Run `make test-db` only for isolated database suites and `make test-all` for the complete handoff gate.
- Test success, retryable failure, ambiguous commit, stale acknowledgement, duplicate replay, bounds, and shutdown paths.
- Use representative history sizes for load plans; tiny local tables are not capacity evidence.
- Run the 25-to-200-client workloads and defined fault injections only on non-production ports and data.

## Formatting and Build Verification

- Run `./scripts/format.sh` for changed C/C++ lines and `./scripts/format.sh --check` to verify.
- Do not commit generated world output, binaries, object files, logs, archives, or runtime player/account data.
- Keep compiler warnings at zero and use the repository sanitizer or Valgrind workflows when the risk warrants them.
- Install or respect `./scripts/install-hooks.sh`; do not bypass repository checks.

## Observability and Privacy

- Record query and Redis counts, latency histograms, and errors by stable site ID without values.
- Correlate external-I/O time with pulse and event-budget metrics.
- Expose queue age, revision gap, last durable revision, journal age, replay state, reconnect state, deadlocks, and lock waits.
- Remove unconditional `/tmp` and per-save traces from normal persistence paths.
- Any diagnostic trace must be explicit, sampled, redacted, non-blocking, size-bounded, and rotated.
- Never commit or disclose credentials, private keys, IPs, password hashes, player descriptions, logs, or player/account data.

## Documentation

- Update `docs/DATABASE.md`, `docs/ARCHITECTURE.md`, README configuration guidance, and the runbook when implemented behavior changes.
- Describe actual execution boundaries, durability guarantees, deadlines, degraded modes, recovery, and operator actions.
- Keep source evidence links and acceptance commands current as code moves.

## Git and Review

- Preserve unrelated worktree changes and avoid destructive Git commands.
- Keep each commit to one logical, reviewable change.
- Never add co-authors, attributions, `Claude-Session`, or signed-off-by lines.
- Review behavior, failure semantics, data migration safety, and observability together.

## Local Development Tools

| Category | Tool | Config or Command |
|----------|------|-------------------|
| Formatter | clang-format | `.clang-format`, `./scripts/format.sh` |
| Linter | g++ strict warning profile | `src/Makefile`, `tests/async/test_compiler_warning_profile.py` |
| Type safety and compiler | g++ C++20 with `-Werror` | `make -C src` |
| Focused tests | Python/Bash | `tests/async/test_*.py`, `tests/async/run_*.sh` |
| Full tests | Make | `make test-all` |
| Observability | Bounded persistence telemetry and redacted logs | `src/persistence_observability.*`, log-hygiene regressions |
| Database tests | Isolated MySQL/Docker | `make test-db` and DB-specific wrappers |
| Git hooks | Repository hook installer | `./scripts/install-hooks.sh` |
| Database | MySQL/MariaDB | `.env`, `migrations/run_migration.sh` |
| Dev server | DurisMUD supervisor | `scripts/start_mud.sh --dev` |

## CI/CD

| Bundle | Status | Workflow |
|--------|--------|----------|
| Code Quality | configured | `.github/workflows/quality.yml` |
| Build & Test | configured | `.github/workflows/build.yml` |
| Security | configured | `.github/workflows/security.yml` |
| Integration | not configured | - |
| Operations | not configured | `.github/dependabot.yml` provides dependency updates only |

## Infrastructure

| Component | Provider | Details |
|-----------|----------|---------|
| Hosting | externally managed | No repository-owned production target is declared |
| Database | MySQL/MariaDB | Required at boot; pooled readiness is exposed without a blocking query |
| Health | DurisMUD WebSocket listener | `GET /health` on `DURIS_WEBSOCKET_PORT` (default 4050); JSON status; `scripts/healthcheck.sh` probe |

## When In Doubt

- Decide from repository and measured development-clone evidence and record material assumptions.
- Preserve live state and retryability over optimistic destructive completion.
- Prefer one durable ordered command over loosely timed cross-system side effects.
- Report any validation that could not be run.

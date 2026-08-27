# Security & Compliance

> Cumulative security posture and GDPR compliance record. Updated between phases via carryforward.
> **Line budget**: 1000 max | **Last updated**: Phase 00 audit (2026-08-27)
>
> Scope: Phase 00 implementation evidence, integrated tests, and static repository
> inspection. This is an engineering readiness record, not legal advice.

---

## Current Security Posture

### Overall: AT RISK

| Metric | Value |
|--------|-------|
| Open Findings | 3 |
| Critical/High | 2 (0 critical, 2 high) |
| Medium/Low | 1 (1 medium, 0 low) |
| Phases Audited | 1 |
| Last Clean Phase | -- |
| Findings Resolved | 7 |

### Existing Safeguards

- [P00] Account and private-chest passwords use bcrypt cost 12; valid legacy values
  have tested upgrade paths and secret comparisons are constant-time.
- [P00] All 124 inspected local base tables use InnoDB, and the principal player-save
  components already share a transaction (`src/sql_player.c:946`).
- [P00] Item/scalar persistence tables have unique dedupe keys, and player item child
  tables use useful foreign keys (`migrations/bootstrap_multithread_safe.sql:797`).
- [P00] Every runtime MySQL connection uses one fail-closed configuration and verifies
  bounded transport plus required session invariants.
- [P00] Persistence failure diagnostics use stable call-site/error metadata without
  SQL text or bound private values.
- [P00] Redis contexts have bounded connection/command behavior and retain dirty state
  instead of synchronously saving full players on the simulation thread.
- [P00] `.env`, deployment certificates, runtime logs, player/account runtime paths,
  SBOMs, and scanner reports are ignored by Git; no secret value was read in the audit.

---

## Findings Register

Cumulative security, integrity, availability, and GDPR findings. Ordered by original
severity; each entry carries its current status.

### Critical / High

- **[P00-S01] Sensitive database values can enter application logs**
  - Severity: High
  - File: `src/sql.c:1819`, `src/sql_persistence_raw.c:33`, `src/sql_player.c:45`, `src/account.c:41`
  - Description: Failed queries log full SQL or a 200-character prefix, while
    unconditional trace files and debug lines record player IDs and pointer values.
    Query text can contain password hashes, confirmation codes, email addresses, IPs,
    player descriptions, messages, and other private values (PRD DB-019).
  - Remediation: Log stable site ID, operation ID, error code, and duration only.
    Remove the ad hoc traces or make diagnostics explicit, sampled, redacted,
    non-blocking, size-bounded, permission-restricted, and rotated.
  - Status: Resolved in Phase 00 Session 01
  - Resolution: Failure logging now emits categorical site, error, and timing metadata;
    raw SQL/ad hoc trace paths are removed and protected by log-hygiene regressions.
  - Opened: P00 (2026-08-26)

- **[P00-S02] Database credentials and transport fail open**
  - Severity: High
  - File: `.env` (local mode), `src/sql.h:7`, `src/sql.c:423`, `src/sql_pool.c:47`
  - Description: Missing configuration falls back to compiled shared credentials; the
    inspected local `.env` is mode 0644; MySQL connections set no connect timeout or
    TLS policy and do not verify all required session invariants (PRD DB-020).
  - Remediation: Restrict local secret files to 0600, require explicit environment role
    and allow-listed DB target, fail closed when credentials are absent, and require
    verified TLS or a protected local socket/tunnel with bounded connect deadlines.
  - Status: Resolved in Phase 00 Session 08
  - Resolution: Runtime roles, targets, credentials, secret-file permissions, bounded
    transport, and required session invariants now fail closed through one connection path.
  - Opened: P00 (2026-08-26)

- **[P00-S03] TLS listener can use a publicly tracked private key**
  - Severity: High
  - File: `src/ssl.c:38`, `certs/localhost.key`
  - Description: When root deployment credentials are absent, the listener silently
    loads the tracked localhost certificate and private key. That key is suitable only
    for local testing; network use permits trivial impersonation.
  - Remediation: Allow the fallback only in an explicit local role bound to loopback.
    Fail closed for network deployments unless an operator-supplied certificate and
    permission-restricted private key pass validation.
  - Status: Resolved in Phase 00 Session 08
  - Resolution: The tracked localhost key is restricted to explicit loopback-local use;
    network listeners require validated operator-owned certificate material.
  - Opened: P00 (2026-08-26)

- **[P00-S04] Persistence can overwrite or destroy newer player state**
  - Severity: High
  - File: `src/redis.c:921`, `src/files.c:1667`, `src/utility.c:1416`
  - Description: Forked saves can commit stale snapshots, terminal paths can extract
    live inventory after SQL failure, and legacy fallback/replay paths do not provide
    uniform revision, reconciliation, or idempotency guarantees (PRD DB-002, DB-005,
    DB-015).
  - Remediation: Retain live and dirty state on failure; use immutable revisioned jobs,
    exact acknowledgements, and a typed checksummed journal whose destination records
    mandatory operation IDs before replay is checkpointed.
  - Status: Open
  - Carryforward: Session 03 retained retryable live state and Session 06 contained
    fork/Redis failure. Phase 01 owns complete revisioned worker and journal replacement.
  - Opened: P00 (2026-08-26)

- **[P00-S05] Economy and ownership updates lack atomic integrity boundaries**
  - Severity: High
  - File: `src/sql_player.c:10814`, `src/epic.c:353`, `src/sql.c:2089`
  - Description: Shared bank balances can be overwritten from stale character state;
    wallet, epic balance, ledger, ownership, and audit effects can commit separately or
    replay without a mandatory unique command ID (PRD DB-006, DB-007, DB-011).
  - Remediation: Commit each critical action once using an idempotent transaction over
    the authoritative balance or owner row, immutable ledger, both affected revisions,
    and any outbox record; publish only the committed result.
  - Status: Open
  - Carryforward: Sessions 05 and 07 fixed frag publication, artifact output, and stale
    absolute bank writes. Phase 02 owns operation-keyed atomic domain transactions.
  - Opened: P00 (2026-08-26)

### Medium / Low

- **[P00-S06] Manual SQL construction depends on unenforced session assumptions**
  - Severity: Medium
  - File: `src/sql.c:1953`, `src/persistence_queue.c:1668`, `src/sql_player.c:4725`
  - Description: Hundreds of formatted queries use multiple escaping conventions, and
    queued escaping assumes backslash behavior while boot does not enforce SQL mode.
    Main, pool, and child connections do not establish one complete session contract.
  - Remediation: Use prepared statements or typed repositories for hot and sensitive
    paths; minimize `CLIENT_MULTI_STATEMENTS`; set and verify `utf8mb4`, time zone,
    isolation level, and SQL mode on every connection.
  - Status: Resolved in Phase 00 Session 08
  - Resolution: Main, pooled, child, migration, and auxiliary connections now share and
    verify the required character set, time zone, isolation, and SQL-mode contract.
  - Opened: P00 (2026-08-26)

- **[P00-S07] Private chest passwords use unsalted SHA-256**
  - Severity: Medium
  - File: `src/sql_player.c:5996`, `src/sql_player.c:6117`, `migrations/bootstrap_multithread_safe.sql:1298`
  - Description: Chest creation and verification use `SHA2(password, 256)` without a
    per-secret salt or adaptive work factor, making weak player-chosen secrets cheap to
    crack after database disclosure.
  - Remediation: Store a versioned adaptive password hash with a unique salt, compare in
    constant time, and rehash legacy chest secrets after successful verification or an
    explicit reset.
  - Status: Resolved in Phase 00 Session 09
  - Resolution: New/reset values use unique-salt bcrypt cost 12; verified legacy SHA-256
    values conditionally upgrade without overwriting a concurrent password change.
  - Opened: P00 (2026-08-26)

- **[P00-S08] Retention and data-subject rights are not implemented end to end**
  - Severity: Medium
  - File: `src/account.c:395`, `src/account.c:2259`, `migrations/bootstrap_multithread_safe.sql:135`
  - Description: The account menu advertises deletion, but both account-deletion
    handlers are empty. No documented access/export, per-user erasure, backup
    propagation, or retention schedule covers account, IP, message, log, PvP,
    progression, economy, and ownership records (PRD DB-018).
  - Remediation: Establish controller-approved purposes, lawful bases, retention and
    exception rules, then implement authenticated export and deletion workflows with
    complete table, cache, journal, log, and backup coverage plus auditable outcomes.
  - Status: Open
  - Progress: Phase 03 Sessions 07-08 add a complete technical lifecycle inventory,
    fail-closed validation, guarded archive schema/state machine, dry-run controls, and
    a policy-disabled scheduler slot. Controller decisions remain pending; no archive,
    export, or erasure mutation is enabled.
  - Opened: P00 (2026-08-26)

- **[P00-S09] Redis failures can compromise availability and persistence behavior**
  - Severity: Medium
  - File: `src/redis.c:145`, `src/redis.c:854`, `src/redis.c:1435`
  - Description: Main and recovery contexts use unbounded `redisConnect`/`redisCommand`
    calls, one dirty-path command runs before its null-context check, and Redis failure
    can trigger synchronous full SQL saves on the simulation thread (PRD DB-003,
    DB-016).
  - Remediation: Apply connect and command deadlines, guard every context before use,
    isolate cache from durability domains, retain dirty state locally, and use bounded
    retry/circuit behavior without synchronous mutation-path fallback.
  - Status: Resolved in Phase 00 Session 06
  - Resolution: Connections and commands are bounded, null contexts are guarded, dirty
    state is recovered, and mutation paths no longer synchronously full-save on failure.
  - Opened: P00 (2026-08-26)

- **[P00-S10] Security policy and dependency automation are placeholders**
  - Severity: Low
  - File: `SECURITY.md:1`, `.github/dependabot.yml:1`, `.github/workflows/build.yml:1`
  - Description: The security policy contains template versions and no reporting
    channel; Dependabot has an empty package ecosystem; CI compiles and tests but has no
    dependency inventory, CVE audit, SBOM, or static security scan.
  - Remediation: Publish a real disclosure policy and supported-version statement,
    remove or repair invalid automation, inventory native/system dependencies, and add
    reproducible dependency and source security checks with triage ownership.
  - Status: Resolved in Phase 00 Session 10
  - Resolution: Actionable reporting, direct dependency inventory/SPDX, immutable
    CodeQL/Trivy CI, triage rules, and explicit unknown coverage are now maintained.
  - Opened: P00 (2026-08-26)

---

## GDPR Compliance Status

### Overall: NON-COMPLIANT

This means the repository lacks enough documented and implemented controls to claim
GDPR readiness; it is not a determination of legal applicability or liability. The
assessment tracks the GDPR principles and rights in the
[official regulation](https://eur-lex.europa.eu/legal-content/EN/TXT/?uri=CELEX:32016R0679).
The European Commission identifies names, email addresses, IP addresses, and comparable
online identifiers as personal data in its
[application guidance](https://commission.europa.eu/law/law-topic/data-protection/information-business-and-organisations/application-gdpr_en).

### Personal Data Inventory

| Data Element | Source | Storage | Purpose | Legal Basis | Retention | Deletion Path | Since |
|-------------|--------|---------|---------|-------------|-----------|---------------|-------|
| Account identity and authentication: account name, email, password hash, confirmation code, login times, donation total | Registration, login, account updates | `accounts`, `account_characters`, account runtime state | Authentication, recovery, account operation | Not documented | Not defined | None complete; account deletion handlers are empty | P00 |
| Network identifiers: IP address, hostname, connection counts, last login | Client connections | `account_ips`, `account_characters.last_ip`, `player_data.last_ip`, `log_entries` | Authentication history, abuse prevention, operations | Not documented | Not defined | No complete per-user path; one FK cascade covers only `account_ips` | P00 |
| Character profile and user-authored content: names, titles, descriptions, introductions, custom messages | Character creation and gameplay input | `player_data`, player subtables, `offline_messages`, PvP records | Core gameplay and communication | Not documented | Active/soft-deleted state; no schedule | Character SQL delete is partial; no authenticated export/erasure workflow | P00 |
| Gameplay, economy, and ownership history: progression, epics, balances, items, lockers, auctions, guilds, rewards | Gameplay events and commands | Player/account tables, `epic_gain`, `progress`, persistence events, locker and auction tables | Gameplay state, reconciliation, fraud and audit history | Not documented | Several tables are append-only with no policy | No documented per-user path; ledger/audit exceptions are undecided | P00 |
| Activity, moderation, and communication records: login logs, messages, PvP logs, chest activity, admin actions | Runtime and administrator actions | `log_entries`, `offline_messages`, `pkill_info`, `private_chest_log`, log files | Operations, moderation, support, audit | Not documented | Not defined | No complete path across database and files | P00 |
| Diagnostics and recovery copies: SQL text, IDs, pointers, fallback records, pfiles, Redis state, backups | Failures, persistence, recovery, operator tooling | `logs/`, `/tmp/garp-item-trace.log`, fallback files, runtime pfiles, Redis, backup/archive locations | Diagnosis, durability, disaster recovery | Not documented | Inconsistent TTL or unbounded; backups unassessed | Manual/incomplete; deletion propagation is undocumented | P00 |

### Compliance Checklist

| Requirement | Status | Notes |
|------------|--------|-------|
| Data collection has documented purpose | FAIL | Purposes can be inferred from code, but no approved processing record or privacy notice was found. |
| Lawful basis is documented per purpose | FAIL | No lawful-basis assessment was found; the inventory does not invent one. |
| Consent obtained when consent is the selected basis | N/A | No processing activity is documented as relying on GDPR consent. |
| Data minimization verified | FAIL | Raw-query/ad hoc persistence traces were removed, but duplicate identifiers and broad histories remain unassessed. |
| Retention limits are defined and enforced | FAIL | Append-only and operational records lack table-by-table schedules and enforcement. |
| Access and export path exists | FAIL | No authenticated, complete data-subject export path was found. |
| Deletion/erasure path exists | FAIL | Account deletion is a no-op and no complete cross-store erasure workflow exists. |
| No personal data in application logs | FAIL | SQL and bound-value leakage is fixed, but ordinary operational/game logs still include names, IPs, and activity without an approved retention boundary. |
| Security of processing is verified | FAIL | Two high integrity findings remain pending the Phase 01 and Phase 02 architecture. |
| Third-party or processor transfers documented | FAIL | Deployment-specific MySQL, Redis, DurisWeb, hosting, and backup boundaries are not inventoried. |
| Breach and vulnerability process documented | PASS | `SECURITY.md` provides private intake, response targets, coordinated disclosure, and a public fallback. |

---

## Dependency Security

### Current Vulnerabilities

The 2026-08-27 local Trivy 0.70.0 run recognized Ubuntu 24.04 and all 19 resolved
direct packages. It reported one unfixed MEDIUM advisory (`CVE-2024-52005`) for the
installed Git package and no fixed HIGH/CRITICAL finding. Transitive packages,
deployment-only services, external infrastructure, and future GitHub CodeQL results
remain `UNKNOWN`; this record does not claim the dependency set is vulnerability-free.

### Audit Status

| Scope | Current State | Status |
|-------|---------------|--------|
| Native and system libraries | Deterministic direct inventory, SPDX 2.3, and recognized minimal Ubuntu scan root | PARTIAL |
| GitHub dependency updates | Weekly valid `github-actions` Dependabot updates | PASS |
| CI security checks | Local contracts, CodeQL C/C++, and pinned Trivy with retained reports | PASS |
| GitHub Actions supply chain | All third-party actions use immutable full commit SHAs | PASS |

---

## Resolved Findings

- P00-S01: sensitive SQL and bound-value persistence logging (Session 01).
- P00-S02: fail-open database credentials and transport (Session 08).
- P00-S03: network TLS fallback to the tracked localhost key (Session 08).
- P00-S06: unenforced connection session assumptions (Session 08).
- P00-S07: unsalted private-chest SHA-256 values (Session 09).
- P00-S09: unbounded Redis failure behavior and synchronous fallback (Session 06).
- P00-S10: placeholder security policy and dependency automation (Session 10).

---

## Phase History

| Phase | Sessions | Security | GDPR | Findings Opened | Findings Closed |
|-------|----------|----------|------|-----------------|-----------------|
| P00 | 10 | AT RISK | FAIL | 10 | 7 |

---

## Recommendations

1. [P01] Replace forked saves with immutable revisioned jobs, exact acknowledgements,
   and a typed checksummed journal; preserve Phase 00 terminal and dirty-state contracts.
2. [P02] Move bank, wallet, epic, ownership, ledger, and outbox effects into idempotent
   transactions with mandatory operation IDs and reconciliation tests.
3. [P03] Approve a real personal-data processing inventory, lawful bases, privacy
   notice, retention schedule, access/export process, and complete erasure workflow.
4. [P01-P03] Do not mark this posture clean until focused tests, privacy log tests,
   non-production fault injection, and security review close every remaining finding.

---

*Updated from the Phase 00 audit; future phases update it via carryforward.*

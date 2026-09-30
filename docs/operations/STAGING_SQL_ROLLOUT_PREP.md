# SQL staging rollout preparation

Updated: 2026-09-30. **Status: experimental review checkpoint; live staging
rollout and production release remain pending.** The saved candidate is on
`Community-Duris/Duris`, branch `experimental-accounting`. SQL is the first
rollout target, with accounting inactive and damaged ownership cases held for
review.

This public document records technical findings and preparation gates. Exact
server identities, service coordinates, backup generation identifiers, source
evidence digests, incident names, and recovery data are retained in the
operator's protected local evidence. Obtain current operational details from
the owner before a rollout. This document does not authorize a migration,
restart, ownership repair, accounting opening, or production change.

## Candidate and staging baseline

The checkpoint includes the `main` refactor at `62a680dea` and subsequent save,
quest, custody, recovery, and schema preparation. The community repository's
default branch is `master`; the review branch includes the intervening source
history as well as the current implementation checkpoint.

The last read-only staging inspection found:

- a clean tracked live checkout with six local fixes absent from the earlier
  shared base;
- MariaDB 10.11.14, a production runtime role, and an immutable migration history
  ending at sequence 45;
- an active user service and hourly backups, with a completed generation but
  no replica or formal restore-drill receipt;
- a native player quarantine archive and persistent PID policy, retained locker
  receipts, and empty active player and critical-command journals in the
  captured generation;
- no configured dedicated restore mount.

Staging's local changes cover SQL worker recovery, retained journal startup,
replay-prefix draining, acknowledgement retries, persistent quarantine/PID
fences, bounded world recovery, and backup inclusion of recovery evidence.
Relevant changes have been integrated into the candidate and checked locally.
Preserve the live checkout and local commits when preparing a rollout; do not
assume a remote branch contains every staging fix.

Staging and production have separate operational identities. Resolve and verify
the exact staging user, service, database, endpoint, and allowed target from
protected configuration before issuing a command. Keep bounded build concurrency
on the shared host. Production changes require separate owner authorization.

## Immutable migration history fork

Candidate and staging migrations **0001–0044 match**. Sequence **0045 conflicts**:
staging applied `0045_item_extra_description_fulltext_unique`, while the
canonical candidate applied `0045_quest_reward_obligation`. IDs and apply/verify
checksums differ. Applying only canonical 0046–0049 is not a valid transition.

The canonical runner refuses the staging history. The explicit append-only
manifest `migrations/migration_manifest.staging_0045.json` preserves the
original 45 files and receipts, then appends the canonical quest, realized-price,
XP-mask, XP-entitlement, and spell-receipt migrations at sequences 46–50.
The canonical manifest appends full-description uniqueness as 0050. Both
completed histories converge on the same 223-table schema on each supported
engine.

The uniqueness migration uses complete descriptions, preserves case/accent
variants and long descriptions sharing a prefix, and refuses exact duplicates
before permanent DDL without deleting evidence. Boot and shell gates accept
only the two exact completed histories and reject changed receipts, state
checksums, and digest expressions. Do not relabel 0045, edit old receipts, or
disable boot verification.

The migration runner holds its advisory lock on one persistent connection with
reconnection disabled. DDL, verification, receipt insertion, and state update
use that connection. Receipt/state recording is atomic; a failed state
comparison rolls back the receipt. Connection loss permanently fences that
runner instance.

## Completed local qualification

### Builds and focused recovery journeys

Both maintained production profiles build. The latest qualified binaries before
this documentation/packaging checkpoint were:

| Backend | SHA-256 |
| --- | --- |
| MariaDB production | `526edd3e87b02613df33c400163acd7395b15a8647fe621ab5f285ec5e289221` |
| Flatfile production | `01399766c69ac2b5df31d0d72a5633a43c0aacc3a77d1684b8079acdd704c1cf` |

SQL and flatfile offering-publication and XP-commit/lost-completion crash
journeys recover through a second cold restart with exact rewards and closed
obligations. Focused native tests cover journal quarantine/PID fences,
allocation refusal, save workers, dispatch, death/XP/spell receipts, custody
write guards, and selected publication paths. These are partial qualifications;
the full writer and integrated gameplay matrices remain open.

### Dual-engine migration and session faults

Disposable MySQL 8.0.46 and MariaDB 10.11.19 fixtures pass canonical/staging
migration, rerun, original-receipt preservation, description preservation,
history/state/expression tamper refusal, and compiled boot checks. Native
faults cover competing runners, stable connection identity across DDL,
failed-state rollback, connection-kill rollback, closed-session refusal,
timeouts, SQL errors, cancellation, and allocation failure.

The database restore qualifier accepts either exact completed history, refuses
partial/mixed/edited/extended histories, and closes its persistent client on
success or refusal. Its history selector is checked against native database
rows as well as the actual compiled boot predicates.

Reproduce the transition and native session checks with:

```bash
python3 tests/async/test_staging_migration_fork_mysql.py
# Narrow migration-session fault proof:
python3 tests/async/test_staging_migration_fork_mysql.py --lock-only
```

### Privately captured staging generation

A completed staging generation was verified read-only and captured into an
ignored, owner-controlled local evidence directory. All 14 files matched its
frozen manifest, and the source manifest was rechecked after capture. This
historical generation is not a fresh cutover backup or a second independent
rollback copy.

An isolated MariaDB 10.11.14 clone restored the captured SQL under a new database
name using a schema-scoped import account. The canonical manifest refused the
fork before changes; the staging manifest appended exactly five migrations.
All original 45 receipts, including timestamps, remained identical. Deterministic
native dumps of player/pet data, descriptions, current custody, ownership
baseline/ledger, death disposition/custody, inbox, currency ledger, and banks
were unchanged. Shell/compiled boot gates passed, and rerunning migration
preserved all 50 receipts. Accounting activation and opening tables stayed empty.

The complete captured generation also passed the backup verifier on a private
Linux copy. A newly built native recovery qualifier passed preflight and drained
checks twice for copied journals, quarantine archive/PID policy, and locker
receipts, with every original recovery file unchanged. No live journal was
replayed. Temporary database containers and volumes were removed.

The upgraded clone retains 417 custody-history cases and 1,901 physical topology
findings over 73,490 payload rows. Categories overlap; they are not a count of
distinct items requiring restitution. No owner candidate was selected, hold
cleared, disposition approved, or historical accounting inferred. Exact per-item
and per-player evidence remains private.

### Minimal isolated service restore

The SQL binary identified above starts against the upgraded captured clone in
the existing isolated mini-world service test. In a separate user/network/PID
namespace it reaches healthy persistence, starts the save/load/critical
pipelines, and shuts down normally with exit status zero. Copied recovery files
remain identical. The integration test now detects missing `iproute2` before
namespace startup. This proof does not cover the full world or existing-character
login/save.

## Remaining first-rollout gates

| Gate | Current state |
| --- | --- |
| Candidate source checkpoint | Saved for experimental review; pin the selected commit and rebuild for deployment |
| Canonical/staging-fork schema and runner | Qualified locally on both engines and on the captured MariaDB clone |
| Captured recovery-file formats | Native copied-file qualification passes; archived cases remain held |
| Minimal clone service boot/shutdown | Passes for the identified SQL binary and captured generation |
| Full-world clone login/save | Pending |
| Integrated disconnect/database interruption/copyover | Pending |
| Quest recovery load budgets | Pending: per-reward SQL queries are undercounted and XP continuation bytes omitted |
| Fresh independent rollback generation | Pending |
| Dedicated staging restore mount and formal backup drill | Pending |
| Complete independent monetary/UID reconciliation | Pending |
| Ownership repair and witnessed accounting opening | Pending; case evidence and reviewed dispositions are incomplete |
| Accounting release validator | Refuses missing executable writer evidence; activation stays guarded |

## Cutover preparation order

1. Preserve live staging fixes and operational identities in the private release
   packet. Pin the review commit, build with bounded concurrency, and run focused
   save, connection, migration, journal, and accounting-inactive checks.
2. Provide the policy's dedicated restore filesystem and complete a verified
   backup drill. Restore a fresh independent generation to a separate staging
   clone with matching MariaDB version and SQL mode. Keep another verified
   generation untouched for rollback.
3. Apply the explicit staging-fork manifest on the clone, preserving the first
   45 receipts. Verify history/schema, journal fences, payload/custody evidence,
   native monetary reconciliation, full-world login/save, restart, database
   interruption, disconnect, and copyover. Prove a second migration run is idle.
4. Review all held cases separately from new regressions. Complete the protected
   cross-backup/journal case register before any ownership repair. Keep
   accounting inactive for the first code rollout.
5. After owner authorization for the concrete staging cutover, take and verify
   a fresh backup, quiesce writers, recheck exact source/database identities,
   apply only rehearsed pending steps, promote the pinned binary, and verify
   process identity, listeners, schema, health, logs, and login/save journeys.
6. On failed migration or persistence checks, restore the complete database,
   journals, and old binary from the verified rollback package. An old binary
   must not be assumed compatible with an upgraded schema.

See [the active accounting plan](../persistence/economy_accounting/FINISH_ACCOUNTING_PLAN.md)
for outstanding implementation and [ownership repair](PRODUCTION_RELEASE_AND_OWNERSHIP_REPAIR.md)
for the protected case register and witnessed opening requirements. Detailed
local qualification reports and raw recovery data are deliberately excluded
from Git; public summaries do not substitute for those deployment records.

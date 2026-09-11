# Full persistence backup and isolated recovery

Issue #83 introduces one policy and generation format for both authoritative
persistence modes. Install and approve the policy before updating a host: the
pre-cycle gate now refuses an unconfigured backup instead of using implicit
retention. Nothing in this PR installs timers or enables a host policy.

## Policy and custody

Copy scripts/backup_policy.example.json to an absolute, owner-only file, assign
the real custodian and paths, and set approved to true only after host review.
The example values approved for this PR are:

| Setting | Example |
| --- | --- |
| Independent capture | Every hour |
| Recovery-point objective | 2 hours |
| Retention | 48 hourly, 14 daily, 8 weekly buckets |
| Local budget | 20 GiB, including publication headroom |
| Required free space | 1 GiB |
| Restore drill | Every week |

The newest generation in each UTC epoch-aligned bucket is retained. Overlapping
tiers share a generation. Always preserve the two newest valid generations.
Rotation happens only after complete publication, fsync, checksum verification,
and the local generation's replica attempt. Failed capture/verification never
prunes prior generations. Before a new capture, generations outside the current
retention tiers may be pruned, but the two newest valid generations are always
preserved. Missed backups never trigger deletion of the last recovery points. This is a
nominal eight-week recovery window with hourly and daily detail, conditional on
successful jobs and sufficient capacity, not a promise that every historical
hour is retained.

Budget for one complete new generation after retaining the required data. The
controller may first remove generations outside the current retention tiers, but
it refuses capture/publication when retained data plus the new generation still
exceeds the budget; it does not sacrifice required generations to make space.
Capacity failures, orphan staging directories, invalid generations, failed
rotation, and an exceeded RPO are errors. Provision measured headroom before activation.

Only the custodian UID may own or access managed roots, generations, policy,
and environment files (0700 directories, 0600 files). Declared forbidden live DB datadirs may belong to the database service UID. Parent directories must
be root/custodian controlled; symlinks, hardlinked files, writable ancestors,
and overlap with any live authority are rejected. Root-owned sticky temporary
ancestors are allowed for disposable tests. The custodian is the trust boundary:
another process running under that UID can alter its backups.

Set BACKUP_POLICY_FILE in the protected environment file. The launcher reads
literal dotenv assignments only; it never evaluates shell commands, expansions,
or substitutions. Quote values containing spaces. Explicitly configure all live
authority roots, plus journal_roots.players and journal_roots.critical.
A configured runtime journal environment path must agree with policy. An empty
journal mapping is only appropriate when both pipelines are explicitly unused,
such as a synthetic fixture.

MariaDB uses one full transactional dump, including schema, migration history,
lifecycle tables, and all runtime tables. Nontransactional tables are rejected.
Coordinate schema migrations/DDL with the backup window; a transactional data
snapshot does not make concurrent DDL safe. Flatfile capture preserves identity
ÃƒÂ¢Ã¢â‚¬Â Ã¢â‚¬â„¢ critical authority ÃƒÂ¢Ã¢â‚¬Â Ã¢â‚¬â„¢ account locking, pending-transaction evidence, and the
complete durable file tree.

Journal trees are copied before the authority snapshot and compared again after
it. A changing/compacted WAL rejects the generation, preventing an old SQL
snapshot from being paired with already-truncated replay evidence. Busy hosts
must arrange a quiet snapshot window or a brief operator-controlled writer
quiescence; repeated churn is an RPO incident, not a successful backup. Restores
copy and replay these same journal directories. Historical generations are
never edited.

## Commands and scheduling

All commands below run as the custodian on a configured host. No command
connects to a game session or promotes a candidate.

    BACKUP_ENV_FILE=/etc/duris/backup.env scripts/backup_pfiles.sh
    BACKUP_ENV_FILE=/etc/duris/backup.env scripts/backup_pfiles.sh status --require-drill
    BACKUP_ENV_FILE=/etc/duris/backup.env scripts/backup_pfiles.sh schedule

The no-argument command remains the pre-cycle safety gate. A not-yet-created flatfile authority on its first boot reports authority_not_initialized and allows initial provisioning; scheduled jobs treat that state as an error, and no verified generation is claimed. The schedule command
uses a separate persisted deadline, so pre-cycle runs do not defer independent
snapshot. It is safe to invoke once per minute. All capture, rotation, restore,
and status operations use an exclusive job lock with a bounded wait; a remaining
busy condition is reported as a fixed error. The systemd backup, health, and drill
units declare mutual conflicts, and the pre-cycle launcher retries a busy backup
before refusing to boot.

Sample inactive systemd units are in deploy/systemd/duris-backup-*. Copy them,
adapt User, WorkingDirectory, ReadWritePaths, paths, and permissions, and connect
OnFailure to the custodian's existing alerting service before enabling the
timers. The backup/health timers evaluate each minute; the drill timer evaluates
hourly and runs only when the configured week has elapsed. Timer persistence
catches missed invocations after host downtime. Test a deliberate invalid
policy and verify that the failure reaches the responsible operator.

stdout/stderr contain JSON with fixed result/error codes, generation IDs,
aggregate age/bytes/counts, and separate replica status. Recovery-point age starts
before journal and authority capture, so dump duration cannot hide an RPO breach. No credentials, account
names, hosts, or player values are telemetry. Alert on any nonzero job result,
rpo_exceeded, capacity failures, interrupted work, or missing/overdue drill
receipts. Monitor timer/unit availability too: a stopped scheduler cannot
report its own failure.

A failed post-publication step leaves a complete generation and preserves prior
ones. Resolve the cause and explicitly retry verification/replication/rotation:

    BACKUP_ENV_FILE=/etc/duris/backup.env scripts/backup_pfiles.sh finalize

Never delete an unexplained staging/trash directory automatically. Inspect
ownership and boundaries, preserve incident evidence, and reconcile it under
the custodian's control. finalize does not overwrite generations.

## Separate/off-host storage

replica_root optionally names an already mounted SSHFS destination with the same
owner-only protections and a single custodian/writer. Other transports fail
closed. Provision SSH host-key verification, a restricted remote account,
protected keys, and mount permissions separately. Provision the remote storage
with the same retention budget and sufficient publication headroom.

The controller copies an immutable generation, syncs it, renames it, re-reads
every checksum, and applies the same retention policy remotely. Any remote
failure is recorded in status.json as a retryable pending result; the verified
local generation remains published, but local rotation waits until replication
succeeds. transport_and_readback_verified means encrypted
transport plus successful remote readback/fsync; it does not prove the remote
operator's media replication, physical custody, or survival of host/storage
loss. Verify those separately with an independent restore from remote media.
A disconnected mount must fail; never replace it with an ordinary local folder.

## Isolated restore and qualification

Install Python 3.11+, MariaDB 10.11 server/client tools, OpenSSL, iproute2, and
util-linux. Build the native verifier and the matching server binaries:

    make -C src
    make -C src PERSISTENCE_BACKEND=flatfile DMS_BINARY="$PWD/bin/server/dms_restore_flatfile"
    python3 scripts/build_restore_qualifier.py

The restore root must be a dedicated mounted filesystem, on a different device
from every configured live authority and the backup root. Its *entire*
filesystem size must not exceed max_bytes; maintain min_free_bytes there.
A bounded tmpfs or a dedicated small recovery volume is suitable. A directory
on the live filesystem is rejected, even if its pathname is different. This
bounds compressed-dump expansion without risking the live filesystem. Mount
provisioning belongs to the operator and is never performed by restore.

Supply independently current erasure evidence, held outside backups/candidates:

    {"version":1,"captured_at":<UTC epoch seconds>,
     "policy_sha256":"<SHA256 of migrations/data_lifecycle_manifest.json>",
     "tombstones":[]}

The erasure custodian must establish this snapshot from the current authoritative
ledger and approved policy, never from the selected historical generation. The
ledger must be freshly captured (no more than five minutes old), and missing,
stale, changed, or policy-mismatched evidence blocks restore. The current project
has no durable source-wide erasure propagation adapter.
Accordingly **any nonempty tombstone ledger blocks the whole restore** rather
than risk resurrecting an erased identity or its indirect references. A
historical tombstone row inside the database dump is not used as a substitute for
that current external evidence. No erasure policy is enabled by the backup policy.
Do not manufacture an empty ledger to pass.

    BACKUP_ENV_FILE=/etc/duris/backup.env scripts/backup_pfiles.sh restore \
      --generation <generation-id> --tombstones /var/lib/duris/erasure-current.json
    BACKUP_ENV_FILE=/etc/duris/backup.env scripts/backup_pfiles.sh drill \
      --tombstones /var/lib/duris/erasure-current.json

Omit generation to select the newest verified generation. Paths and existing
databases cannot be supplied as restore targets: every candidate is newly
created below the dedicated restore mount. The legacy flatfile wrapper now
accepts a generation ID and current erasure evidence; its old arbitrary
raw-copy target interface is intentionally closed.

Qualification requires the generation's exact runtime schema manifest. To
restore an older schema, use its matching reviewed code/toolchain in a separate
recovery workspace, then plan a separate migration on another candidate.
Never bypass the compatibility check or edit backup metadata.

MariaDB restore initializes a new private datadir with networking disabled,
imports through a schema-only account with no global/FILE privileges, validates
the runtime schema, recomputes the complete migration history, and reconciles
account/character, wallet, bank, and epic evidence. Runtime boot uses only the
new socket. Flatfile restore verifies copied bytes before mutation, runs native
authority replay, and validates existing account, snapshot, and world catalog bytes.
Full player/domain loads run after WAL replay, allowing a durable first snapshot
to materialize its missing projection. These loads reject lossy topology repair.
Before boot, both journal types are scanned with the production codecs. Any
corrupt/unsupported frame, quarantine evidence, or interrupted temporary journal
blocks qualification. Both modes then boot the matching server against copied mini-world assets in a
new user/network/PID namespace, exercise HTTP readiness, reject persistence startup
failure messages, wait for both journals to drain, and require clean shutdown.
Native postflight requires zero remaining records and no corruption/quarantine;
authority reconciliation runs again after replay. Namespaces must be available;
there is no fallback to a host-network boot.
The server executable and qualification script are copied into the private
candidate before entering the namespace, so recovery also works from a checkout
under another user's private home. Include one server executable per retained
candidate in the recovery filesystem's capacity budget.

QUALIFIED.json is written last and is evidence about this isolated candidate,
not deployment approval or a full-world gameplay/balance certification.
Manual candidates, including failed ones, remain private for diagnosis and
consume the bounded recovery filesystem. Automated drills remove candidates,
credentials, and detailed logs and keep only redacted drill.json evidence.
Review the recorded generation/result and schedule at least weekly.

## Incident selection, cutover, and rollback

1. Fence access and writes using the actual host supervisor; preserve incident
   logs and current authoritative state. Identify the last acceptable recovery
   point from verified generation timestamps, expected data loss, and erasure
   requirements. Record the selected ID and the custodian's decision.
2. Restore and qualify into isolation. Investigate all failures. Compare
   representative account, character, inventory, ledger and world state with
   independent incident evidence; a checksum alone proves neither correctness
   of the source nor the desired incident time.
3. Recheck the current erasure ledger immediately before any cutover. Prepare
   an explicit promotion plan covering the selected authority, journal paths,
   matching binary/schema, caches, listeners, and old-state rollback location.
   Stop writers, preserve a fresh full backup, and obtain host-owner cutover
   approval. These tools deliberately provide no production promotion command.
4. Publish one complete authority/candidate, rebuild caches only from that
   authority, perform load/value checks, then reopen access under observation.
   If validation fails, stop again and return the *whole* previous authority,
   journal set and matching binary/configuration; never mix generations.

Exceptional selective extraction is a private forensic operation on another
qualified clone. Review identity/ownership/value dependencies and record any
approved reconciliation separately. Do not inject an isolated character file
or account row into production, create per-account archives, or treat selective
recovery as a gameplay feature.

## Verification

    python3 tests/async/test_persistence_backup.py
    python3 tests/async/test_backup_pfiles.py
    python3 tests/async/test_flatfile_backup_manifest.py
    python3 tests/async/test_flatfile_launcher.py
    DURIS_RUN_BACKUP_INTEGRATION=1 python3 tests/async/test_persistence_backup_integration.py

The integration test uses synthetic identities, private MariaDB daemons, a
bounded temporary restore mount, and isolated service namespaces. It requires
mount/unshare privileges; run on a disposable Linux host/container (for Docker,
CAP_SYS_ADMIN and an appropriate seccomp profile). No production environment,
credentials, existing database, or live game connection is used.


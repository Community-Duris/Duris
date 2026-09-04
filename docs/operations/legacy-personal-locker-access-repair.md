# Legacy personal-locker access repair

Issue #122 records one imported personal locker with 21 retained items and no
legacy visitor grant. The runtime owner path now validates its stable PID,
current account-character mapping, and racewar side. The repair below adds the
current account as a redundant, explicit visitor grant so the rollout does not
depend on the new predicate alone.

No private locker name, PID, or account name belongs in this repository. Obtain
the exact target from the restricted import audit immediately before the
rollout, and do not infer it from the display name.

## Rehearsal

The isolated MySQL rehearsal creates a production-shaped personal locker with
exactly 21 items, verifies the owner mapping, applies the grant twice, and proves
that the payload is unchanged:

```sh
tests/async/run_legacy_personal_locker_access_repair_mysql.sh
```

## Target check

Set the private values only in the operator environment. `--check` is read-only
and fails unless exactly one personal locker has the expected owner, current
unblocked same-side mapping, and item count.

```sh
export LOCKER_REPAIR_NAME='<exact-player>.locker'
export LOCKER_REPAIR_OWNER_PID='<exact-pid>'
export LOCKER_REPAIR_EXPECTED_ITEMS=21
migrations/repair_legacy_personal_locker_access.sh --check
```

For a non-development database, the command prints the exact target-bound
`LOCKER_REPAIR_PRODUCTION_ACK` value it requires. Supplying that value is an
operator confirmation; it is not permission for an automation agent to touch
production.

## Backup-first apply

Choose a new absolute backup path on encrypted operator-controlled storage.
The script writes mode-0600 SQL containing all pre-existing access rows for the
target before it performs the idempotent insert.

```sh
export LOCKER_REPAIR_BACKUP='/secure/operator/path/locker-access-before.sql'
migrations/repair_legacy_personal_locker_access.sh --apply
migrations/repair_legacy_personal_locker_access.sh --check
```

The final check must report `owner_path_ready=1`,
`visitor_grant_ready=1`, and `item_count=21`. Then verify explicit named entry
with the owner character while preserving the existing occupancy-privacy and
opposite-side checks. If verification fails, stop the rollout, retain the
backup, and remove only the exact newly inserted `(owner, visitor)` row after
comparing it with the backup; do not rewrite locker or item ownership.

# Reviewed balance application and rollback contract (#275)

This package is the administrator/config-boundary contract that follows the
#274 shadow recommendation. `scripts/telemetry/balance/application.py` is a
pure evaluator: it returns a safe-boundary intent and an audit record, but it
performs zero writes, opens no database or network connection, calls no
gameplay calculation, and cannot activate production.

A future config owner may consume an accepted intent only after a separate
production approval. The intent's `requires_config_owner_commit=true` field
is deliberate: it prevents an offline evaluation result from being mistaken
for a completed live mutation.

## Command input

An administrator command contains:

- the exact #274 shadow `proposal`, including recommendation ID, policy
  version, report generation, config generation, evidence fingerprint, expiry,
  current value, proposed value, and `shadow_only`/non-application constraints;
- `current_config` and `expected_config`, each with the target parameter,
  generation, monotonic revision, and fixed-point value;
- an explicit `approval` with a redacted actor token, action ID, experiment ID,
  reviewed policy version, and approval timestamp;
- `operation=apply` or `operation=rollback`;
- bounded action history, `now_utc`, `kill_switch`, and an explicit
  `application_enabled` control.

The target is still only `payout.epic.zone.alignmentMod`, in the same
0..1000 milli-fraction encoding as #274. Raw account/character/IP/chat/
location/credential fields are rejected. Every proposal must remain
`shadow_only=true`, `can_apply=false`, and mutation-free at the source; the
separate administrator approval is the only way to produce a safe-boundary
intent.

## Apply behavior

An `apply` command is accepted only when:

1. the proposal is a recommendation, not a hold/abstention;
2. its expiry is after `now_utc`;
3. the current and expected config parameter, generation, revision, and value
   are identical;
4. the proposal's current value and config generation match the expected
   config;
5. the explicit approval is valid and uses the same policy version;
6. the kill switch is off and the application boundary is enabled; and
7. the target value remains inside its fixed bounds.

The result is `accepted_for_safe_boundary` with a next revision, previous and
proposed values, a typed application intent, and an audit record. The result
still says `applied=false` and `writes_performed=0`; a real config owner must
commit the next snapshot and emit its effective-config event separately.

A concurrent edit changes the revision/value/generation and returns
`rejected` with `stale_config` and the unchanged current value as the stable
fallback. Expiry, kill switch, disabled boundary, policy mismatch, invalid
proposal, and missing approval also reject without a write.

## Idempotency and rollback

The command fingerprint includes operation, action ID, recommendation and
evidence identity, exact proposal values, expected config snapshot, and
rollback source. Replaying the same action ID and fingerprint returns
`idempotent_replay` with the original audit record and zero writes. Reusing an
action ID for a different proposal/config returns `action_id_conflict`.

Rollback requires a new approved action and a `rollback_of_action_id` that
refers to an accepted apply audit record. The current config must still hold
the applied value from that record. The result restores the recorded previous
value at the next revision and emits a rollback audit record. Missing or
non-current sources reject with `rollback_source_missing` or
`rollback_source_not_current`; an unrelated concurrent edit is never
overwritten.

Every accepted or rejected result carries stable fallback data and explicit
constraints:

```text
safe_boundary_intent_only = true
writes_performed         = 0
gameplay_mutation        = false
production_activation    = false
sql_or_network           = false
```

The audit record retains the prior/applied value, exact report generation and
config generation/revision, policy/evidence identity, actor/action/experiment
tokens, expiry and issuance time. Rejected commands retain reason codes for
administrator review; rejected input never turns into a guessed value.

## Operational boundary

The `application_enabled` flag and kill switch are pure command inputs for
offline/authorized-boundary tests. This package does not read the live
property file, change a property, add an admin command, or enable automatic
application. A future production rollout must separately document owner
approval, experiment/rollback criteria, effective-config event reconciliation,
and the authorized persistence/config integration. Analytics outage remains
independent of gameplay.

## Offline validation

Run:

```sh
python3 tests/async/test_telemetry_balance_apply_contract.py
```

The fixtures cover accepted intent/audit creation, exact replay idempotency,
action-ID conflict, stale concurrent edits, expiry, kill switch, disabled
boundary, rollback restoration, missing rollback source, raw-field rejection,
and deterministic CLI output. They are synthetic and do not perform live
application, production rollout, migration, load, or database work.

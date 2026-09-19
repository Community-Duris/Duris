# Guided player-death restitution

This guide is the operator-facing contract for the live native `restitution`
command. It is a bounded handoff from the protected SQL-derived tool into the
single game-thread command path. It does not approve hand-written payloads,
perform synchronous SQL, or automatically reimburse a player.

## Boundary and authorization

The protected tool owns evidence capture, plan construction, review, explicit
approval, and canonical export. The game command only accepts the exported
canonical bytes. The staff actor used for `export` must be the same exact actor
used for `begin`, every `chunk`, `commit`, and `status`; it must pass the native
staff level gate. Output never includes player IDs, item UIDs, payload bytes,
credentials, or database evidence.

The supported live-runtime mode is target-scoped: the recipient must be fully
offline, have no pending save, and acquire the recipient save/login fence before
coordinator admission. It does **not** stop the whole server. Offline SQL
application/quiescence is a separate protected tool mode and is not performed by
this in-game command.

## Copyable workflow

Use a private owner-only directory for artifacts. The preparation commands below are
read-only and select the native live-runtime mode explicitly:

```sh
python3 scripts/player_death_restitution.py --env-file /secure/duris.env \
  inspect --pid <PID> --death-revision <DEATH_REVISION> \
  --recipient-pid <PID> --artifact /secure/restitution/inspection.json

python3 scripts/player_death_restitution.py \
  plan --inspect /secure/restitution/inspection.json \
  --preparation-mode native --artifact /secure/restitution/plan.json

python3 scripts/player_death_restitution.py \
  export --plan /secure/restitution/plan.json \
  --inspect /secure/restitution/inspection.json \
  --artifact /secure/restitution/staff-payload.json \
  --approve --actor <STAFF_ACTOR> --reason death_restitution_review
```

For a production-classified target, first create a protected `target-info` artifact
with the exact database-name confirmation and live server fingerprint, then pass that
artifact to both `inspect` and `plan`. Native preparation does not create a backup,
require an offline proof, stop/mask a service, or bind a stopped maintenance boundary:

```sh
python3 scripts/player_death_restitution.py --env-file /secure/duris.env \
  target-info --confirm-production-target <EXACT_DB_NAME> \
  --artifact /secure/restitution/target-info.json

python3 scripts/player_death_restitution.py --env-file /secure/duris.env \
  inspect --target-info /secure/restitution/target-info.json \
  --confirm-production-target <EXACT_DB_NAME> \
  --expected-fingerprint <FINGERPRINT_FROM_TARGET_INFO> \
  --pid <PID> --death-revision <DEATH_REVISION> --recipient-pid <PID> \
  --artifact /secure/restitution/inspection.json

python3 scripts/player_death_restitution.py \
  plan --inspect /secure/restitution/inspection.json \
  --target-info /secure/restitution/target-info.json \
  --preparation-mode native --approve-production \
  --artifact /secure/restitution/plan.json
```

The native plan is `applyable=false` and `exportable=true` only after all exact
recipient, revision, custody, evidence, and production-approval gates pass. An
explicit native plan cannot be sent to SQL `apply` or `mark-verified`.

The offline SQL mode is separate: use `--preparation-mode offline-sql` with the
existing target-info, backup receipt, stopped maintenance boundary, offline proof,
and apply/verification gates. Do not use a native plan as an SQL mutation artifact.

Review the protected plan and payload. Do not edit `canonical_hex` or copy only
that field into a new artifact. In the normal game client, use the commands in
this order:

```text
restitution help
restitution begin
restitution chunk <CHUNK_1_FROM_staff-payload.json>
restitution chunk <CHUNK_2_FROM_staff-payload.json>
# ...one command for every exported chunk, in order...
restitution commit
```

The exporter emits byte-aligned chunks of at most 1004 hex characters. Native
staging accepts at most 512 KiB of encoded command bytes and at most 1045 chunks.
The command reports accepted chunk count/byte count and remaining capacity after
each accepted chunk. A chunk is not durable merely because it was accepted.

The commit response includes the operation identity when one exists. Copy that
identity exactly and use it for reconnect-safe status checks:

```text
restitution status <OPERATION_ID_FROM_commit>
```

To discard an in-memory staging session before commit:

```text
restitution abort
```

The Python helper below only prints game commands; it does not connect to or
submit to the game:

```sh
python3 -c 'import json; p=json.load(open("/secure/restitution/staff-payload.json")); print("\\n".join("restitution chunk "+c for c in p["chunks"]))'
```

## State machine

| State | Meaning | Next safe action | Retry rule |
|---|---|---|---|
| `read-only-preparation` | `inspect`, `plan`, and `export` are protected artifacts; no game submission | Review exact evidence, plan, actor, reason, and canonical export | Rebuild from fresh evidence if stale; never hand-edit |
| `staging` | Actor-bound bytes exist only in game memory | Continue the same ordered chunks or `abort` | Corrected chunk retries are safe; `begin` refuses when data exists |
| `canonical-validation` | The complete staged bytes are being decoded and round-tripped | Wait for the commit result | Do not retry a rejected noncanonical payload unchanged |
| `admission` | Actor, source/deadline, canonical bytes, recipient fences, and coordinator admission are being checked | Use the returned operation identity and `status` | Do not submit a duplicate operation identity |
| `awaiting-durability` | Coordinator retained the operation but journal append/fsync is not confirmed | Poll `status` | Do not retry; the recipient fence remains held |
| `durable-execution` | Journal admission is durable and execution/publication is not complete | Poll `status` | Do not retry; wait for completion |
| `journal-uncertain` | The journal outcome is ambiguous | Use the authorized reconciliation/recovery path, then `status` | **Do not retry. Preserve the fence and operation identity.** |
| `durable-receipt-unverified` | A durable completion/receipt is available | Run the protected exact read-only verification from the same plan | Do not resubmit; a receipt is not proof of target readback |
| `terminal-failure` | The operation ended without a successful delivery claim | Preserve the failure and follow reconciliation/rebuild guidance | Do not reuse the same payload or identity |

`accepted`, `queued`, `attached`, and `durable` are transport/admission states.
None means `delivered` or `verified`.

## Responses and refusal handling

Every response identifies a phase, a next action, and retry guidance.
Important distinctions are:

- **Staging accepted**: only bounded in-memory transport progress changed.
- **Canonical validation refused**: no recipient fence or coordinator submission
  occurred; rebuild `inspect -> plan -> export` rather than editing bytes.
- **Recipient online/pending save/fence unavailable**: no operation was admitted;
  wait for the target boundary and retry only when it is clear.
- **Coordinator unavailable/overloaded/journal failure**: no successful delivery
  exists; resolve the operational condition before a new attempt.
- **Duplicate operation**: the existing operation identity is returned; status it
  instead of submitting again.
- **Journal uncertainty**: the recipient remains fenced. Do not retry, even if no
  completion message has appeared.
- **Durable receipt**: the command reports `delivery=not_verified` and directs the
  operator to the protected exact verification path. The native live command
  does not claim `delivered` or `verified` from admission, receipt, or an
  `already_applied` completion alone.

A duplicate `begin` never clears staged work. Continue the existing session or
explicitly `restitution abort`. `abort` discards only the actor's in-memory
staging; it never cancels or rewrites an already submitted operation.

Malformed, non-hex, odd-length/incomplete, oversized, and no-active-staging
failures do not append data. Chunks have no user-supplied sequence metadata: the
only supported order is the order printed by the protected exporter.

## Reconnect and verification boundary

Staging and the bounded operation-status projection are process-local and
actor-bound. A staff descriptor may disconnect and reconnect to the same running
server, then use `restitution status <operation-id>` without resubmitting. The
status projection contains only the operation identity and phase metadata; it
cannot enumerate another actor's operation.

A process restart can discard the in-memory staging/status projection even though
the durable journal/receipt remains authoritative. If status says the operation
is outside the runtime retention window, use the protected read-only verification
command with the original plan and its approved target policy:

```sh
python3 scripts/player_death_restitution.py \
  verify --plan /secure/restitution/plan.json
```

Supply the same approved environment/target-policy arguments required by the
protected plan. For a production **native** plan, first create a fresh protected
`target-info` artifact while the approved service boundary is stopped and masked,
then pass that artifact and the exact target/fingerprint and maintenance arguments
to `verify`:

```sh
python3 scripts/player_death_restitution.py \
  target-info --confirm-production-target <EXACT_DB_NAME> \
  --maintenance-kind systemd --maintenance-id duris-mud-production.service \
  --artifact /secure/restitution/verify-target-info.json

python3 scripts/player_death_restitution.py \
  verify --plan /secure/restitution/native-plan.json \
  --target-info /secure/restitution/verify-target-info.json \
  --confirm-production-target <EXACT_DB_NAME> \
  --expected-fingerprint <FINGERPRINT_FROM_VERIFY_TARGET_INFO> \
  --maintenance-kind systemd --maintenance-id duris-mud-production.service
```

This native fallback is still a stopped/masked, exact read-only SQL readback; it
is not a live-native verification path. The fresh target-info must bind the same
production name and server fingerprint as the original native plan and its
validated maintenance boundary must still match at verification time. Native
read-only verification does not require or inspect a plan-bound backup receipt,
convert or replan the artifact, write a receipt status, or accept `--mark-verified`.
For an `offline-sql` plan, the existing plan-bound backup, stopped boundary, and
`--mark-verified` offline-proof/approval gates remain unchanged. Do not run either
route while unrelated players are online or turn a queue, receipt, or runtime
completion into a delivery claim. The exact target readback plus receipt, item, ownership, runtime,
artifact, and inventory readback must pass before recording success; otherwise
record refusal/uncertainty.

The normal game protocol is therefore:

```text
restitution status <OPERATION_ID>
# if phase=journal-uncertain: do not retry; use reconciliation/recovery
# if phase=durable-receipt-unverified: run protected verify from the same plan
# only protected exact readback may establish final success
```

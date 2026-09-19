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

Use a private owner-only directory for artifacts. These preparation commands are
read-only until the separately authorized apply/verification step:

```sh
python3 scripts/player_death_restitution.py --env-file /secure/duris.env \
  inspect --pid <PID> --death-revision <DEATH_REVISION> \
  --recipient-pid <PID> --artifact /secure/restitution/inspection.json

python3 scripts/player_death_restitution.py \
  plan --inspect /secure/restitution/inspection.json \
  --artifact /secure/restitution/plan.json

python3 scripts/player_death_restitution.py \
  export --plan /secure/restitution/plan.json \
  --inspect /secure/restitution/inspection.json \
  --artifact /secure/restitution/staff-payload.json \
  --approve --actor <STAFF_ACTOR> --reason death_restitution_review
```

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

The exporter emits byte-aligned chunks of at most 1022 hex characters. Native
staging accepts at most 512 KiB of encoded command bytes and at most 1027 chunks.
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
protected plan. That verification must perform the exact target readback before
any operator records success. If it cannot verify, record refusal/uncertainty;
do not turn a queue, receipt, or runtime completion into a delivery claim.

The normal game protocol is therefore:

```text
restitution status <OPERATION_ID>
# if phase=journal-uncertain: do not retry; use reconciliation/recovery
# if phase=durable-receipt-unverified: run protected verify from the same plan
# only protected exact readback may establish final success
```

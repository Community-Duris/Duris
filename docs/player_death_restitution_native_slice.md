# Native death-restitution operator handoff

This document describes the supported combined-recovery handoff into the native
`restitution` staff command. It is a bounded per-player operation, not a bulk
maintenance writer or a complete player-snapshot replacement.

## Supported native boundary

- One SQL-derived plan addresses one source player and that same player as the
  recipient. It carries one to 64 candidate item records.
- The operator tool reads the death disposition, retained custody, current UID
  ownership, player save/owner revisions, and artifact authority from SQL.
- The plan preserves the authoritative evidence digest, actor/reason, exact UID
  ownership fences, item-state projection, and original item payload. It never
  accepts a hand-written canonical blob.
- The native repository revalidates the decoded plan and the live SQL evidence
  in its transaction: death payload digest, source owner revision, target save
  revision, UID/root/parent/item revisions, quarantine state, vnum, custody,
  duplicate-delivery absence, and artifact authority.
- The staff command accepts only a canonical critical command with
  `source_site=operator_repair` and `deadline_class=interactive`. It verifies
  staff authorization and a byte-for-byte canonical codec round trip before it
  acquires the recipient save/login fence or submits to the coordinator.
- The fence remains held until the normal critical completion or abort path
  resolves the submission. An ambiguous coordinator result is not treated as a
  successful delivery.

The existing generic C command builder intentionally retains its own
`recovery`/`recovery` source/deadline semantics. It is not silently rewritten.
The operator export below is the explicit `operator_repair`/`interactive`
bridge for the restricted staff command.

## Operator workflow

Run these commands from the repository root. Use an owner-only working
directory for the JSON artifacts; the tool creates artifacts with mode 0600.
`--env-file` is global and must appear before `inspect`; planning and export
read only the protected local artifacts and do not need database credentials.

### 1. Capture authoritative evidence

```sh
python3 scripts/player_death_restitution.py --env-file /secure/duris.env \
  inspect --pid <PID> --death-revision <DEATH_REVISION> \
  --recipient-pid <PID> --artifact /secure/restitution/inspection.json
```

For a production target, bind the inspection to the separately verified target
probe and server fingerprint using `--target-info`,
`--confirm-production-target`, and `--expected-fingerprint` as required by the
normal target policy.

### 2. Build the read-only plan

```sh
python3 scripts/player_death_restitution.py \
  plan --inspect /secure/restitution/inspection.json \
  --preparation-mode native --artifact /secure/restitution/plan.json
```

For production, also supply the same protected `--target-info` artifact and
`--approve-production`. Native plans remain `applyable=false`; approval and
eligibility permit `exportable=true`, not offline SQL mutation. See the
[guided workflow](operations/RESTITUTION_GUIDE.md) for the full target-pin sequence.

Review `plan.json` before approval. An artifact is eligible only when its
identity, domain/legacy authority, and timer evidence reconcile. If historical
remaining lifetime is unavailable, use a separately recorded UID-specific
approval; do not invent a timer or use a full-reset fallback:

```sh
python3 scripts/player_death_restitution.py \
  plan --inspect /secure/restitution/inspection.json \
  --preparation-mode native --artifact /secure/restitution/plan.json --overwrite \
  --artifact-timing-compensation <ITEM_UID>=<SECONDS>:<APPROVAL_REFERENCE>
```

Use `--approve-artifact-reconciliation` only for the evidence-backed
reconciliation described in the plan. The native handoff rejects plans with
unresolved candidates, a missing native revision fence, a recipient different
from the source player, or more than 64 candidates.

### 3. Export the approved canonical live payload

Approval is explicit and binds the exact plan/inspection pair, actor, reason,
UID-specific artifact timing decisions, canonical command bytes, and an
approval digest:

```sh
python3 scripts/player_death_restitution.py \
  export --plan /secure/restitution/plan.json \
  --inspect /secure/restitution/inspection.json \
  --artifact /secure/restitution/staff-payload.json \
  --approve --actor <STAFF_ACTOR> --reason death_restitution_review
```

`export` re-derives the plan from the protected inspection and refuses a stale
inspection, changed evidence digest, changed plan digest, non-SQL plan,
non-exportable native plan, or altered UID fence. The resulting protected artifact has
`source_site` `operator_repair`, `deadline_class` `interactive`, the exact
`canonical_hex`, bounded `chunks`, and an `artifact_timing_approvals` map keyed
by item UID. Do not edit or copy only the hex field into a new file.

### 4. Submit through the production staff command

The game command reader accepts at most 1023 characters per complete input line.
The exporter emits at most 1004 hex characters per chunk, leaving room for the
18-character `restitution chunk ` prefix and keeping each chunk byte-aligned.
The 512 KiB payload ceiling therefore permits up to 1045 chunks. In the staff
client, start staging, execute every line printed by the following command in
order, then commit:

```sh
restitution begin
python3 -c 'import json; p=json.load(open("/secure/restitution/staff-payload.json")); print("\\n".join("restitution chunk "+c for c in p["chunks"]))'
restitution commit
```

The Python command prints game commands; it does not submit them to the game.
If staging must be abandoned, use:

```text
restitution abort
```

The actor supplied to `export` must be the same staff identity used for
`begin`, every `chunk`, and `commit`, and must satisfy the native staff level
gate. The command's canonical source/deadline are checked again by the native
staff boundary; changing generic recovery-builder metadata cannot bypass it.

## Rejection and retry rules

- A stale inspection is rejected before export when its evidence digest no
  longer matches the plan. A plan that is manually edited is rejected by its
  protected plan digest and by export's re-derived-plan comparison.
- A plan can still be rejected after export by the native repository if the
  live death, custody, owner, save, artifact, or delivery rows changed. Capture
  a new inspection and build a new plan; never reuse the old payload.
- Unauthorized staff, online/pending-save recipients, unavailable fences,
  malformed chunks, noncanonical bytes, duplicate operations, and coordinator
  failures are rejected without treating the operation as delivered.
- Currency, unsupported complete snapshots, wildcard revisions, missing item
  metadata, and artifact evidence without either historical timing or a
  UID-specific approval remain outside this native slice.

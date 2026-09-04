# Legacy membership reconciliation

The 2026-09-03 target-wins import deliberately cleared 176 nonzero legacy
association IDs and 190 nonzero legacy guild-status bitsets. Those numbers and
bit positions belonged to a different world definition. This runbook provides
the only supported route from that quarantine decision to a proposed repair.
It never changes current guild or association definitions and never treats a
legacy number as identity.

`scripts/reconcile_legacy_membership.py` is a dry-run classifier and protected
plan writer. It emits aggregate counts to standard output. Player-level
evidence and plans stay in owner-only files outside the repository. It does not
connect to a database or execute DML.

## Classification contract

Each reset is represented once, as either an `association` or `guild` row. A
mapping is `uniquely_mappable` only when both of these independently point to
the same active current definition:

- the canonical legacy name, normalized with the documented source rules; and
- membership history from the frozen source and import record.

The protected artifact records those matches as keyed SHA-256/HMAC references,
not player names. The referenced current definition must be present in the
same snapshot. `legacy_numeric_id` and `numeric_definition_ref` are retained
only to demonstrate the old collision; the classifier deliberately never
uses them. A current membership in another definition is `conflicted`. An
inactive/no-successor definition is `obsolete`. Missing or non-unique semantic
proof is `insufficient_evidence`.

For a uniquely mapped row, current authority still wins:

- Existing membership in the proven target produces
  `keep_current_authority`; its current rank is unchanged.
- A missing association membership may produce `set_player_association`.
- A missing guild membership may produce `insert_guild_member` only when a
  non-administrative rank in the current `guild_ranks` snapshot is proven.
- Association prestige is always unchanged. Raw legacy guild-status bits are
  never copied.
- An administrative rank or `administrative_permissions: restore` blocks the
  row. Existing current administrative authority may be preserved, but this
  workflow cannot create or elevate it.

Every row carries a completed effect review for membership, rank, association
prestige, locker access, profile/guild display, forum ACLs, and administrative
permissions. A `conflict` in any effect makes the row conflicted. Before the
plan can be ready, every non-restorable row needs a permanent protected
disposition of `leave_unrestored` or `player_support`, with an evidence
reference. A disposition attached to a `uniquely_mappable` row is invalid and
aborts the entire run. To withhold that restoration, correct or extend the
semantic evidence so the classifier reaches a supported non-restorable class;
do not add a disposition to an otherwise mappable row. This is the durable
support route for a returning player; operators must not improvise a numeric
reassignment.

## Protected evidence

Create the evidence on a fresh production clone from the frozen import stage
and the same-transaction snapshots of `associations`, `guilds`,
`guild_members`, `guild_ranks`, and affected `player_data` rows. The top-level
JSON contract is:

```text
version: 1
source_stage_fingerprint: sha256
definitions_fingerprint: sha256
authority_snapshot_fingerprint: sha256
unchanged_target_fingerprint: sha256
cross_repo_contract_ref: sha256
current_definitions: [definition records]
rows: [one record for each of 176 association and 190 guild resets]
```

A definition record has `definition_ref`, `domain`, current `numeric_id`,
`active`, and current rank records (`rank_ref`, `rank_index`,
`administrative`). An association has no ranks. Each row has:

```text
row_ref, player_ref, domain, legacy_numeric_id, numeric_definition_ref
legacy_definition_state, legacy_name_ref
canonical_name_matches[], membership_history_matches[]
requested_rank_ref
effects {membership, rank, association_prestige, locker_access,
         profile_display, forum_acl, administrative_permissions}
current_authority [{definition_ref, rank_ref}, ...]
evidence_ref
```

References must use a run-specific keyed HMAC prepared and stored with the
backup evidence. Do not use a bare hash of a player or guild name. The evidence
builder computes each row's `evidence_ref` as
`HMAC-SHA-256(run_secret, b"legacy-membership-disposition-v1\0" + canonical_row_json)`,
where `canonical_row_json` is UTF-8 compact sorted-key JSON for every row field
except `evidence_ref`. The disposition copies that exact reference, and the
planner rejects stale or mismatched decisions before planning. Every input file
must be an absolute-path, owner-owned regular file with mode `0600`; symlinks,
duplicate JSON keys, unknown fields, invalid cross-domain references, and
oversized files fail closed. The output plan is created as a new `0600` file in
an existing owner-only directory with no symlink traversal and binds every
snapshot and input digest.

Disposition JSON uses this shape:

```json
{"version":1,"dispositions":[
  {"row_ref":"<sha256>","decision":"leave_unrestored","evidence_ref":"<sha256>"}
]}
```

## Aggregate dry run

The issue-specific totals are defaults and form a hard gate:

```sh
chmod 600 /protected/reconciliation-evidence.json
chmod 600 /protected/reconciliation-dispositions.json
python3 scripts/reconcile_legacy_membership.py \
  --evidence /protected/reconciliation-evidence.json \
  --dispositions /protected/reconciliation-dispositions.json \
  --plan-output /protected/reconciliation-plan.json
```

The command reports only aggregate association and guild totals in the four
classes, required-disposition count, and status. It exits nonzero and refuses
the plan unless there are exactly 176 association rows, exactly 190 guild rows,
and no missing permanent disposition. `--require-association-count` defaults to
176 and `--require-guild-count` defaults to 190. The flags allow separately
approved reuse with different retained sets; do not override them for this
issue. Record the exact invocation and both effective count values so any
override is visible. Archive those values with the inputs, output, stdout, and
SHA-256 values in the change record. Do not add them to Git or an issue.

## Clone rehearsal and approval gate

Production execution remains prohibited until every item below is attached to
the operator change record:

1. A restorable backup taken immediately before the proposed change, plus a
   successful restore rehearsal.
2. Explicit owner/operator approval naming the plan SHA-256 and exact affected
   row counts.
3. A fresh production clone whose frozen-stage, definition, authority, and
   unaffected-target fingerprints match the plan.
4. DurisMUD acceptance for login, association commands, current rank behavior,
   prestige lists, association/guild lockers, and administrative checks.
5. DurisWeb acceptance (counterpart issue #16) for profile/guild display,
   forum ACL behavior, and confirmation that no administrative permission is
   gained. Record that acceptance as `cross_repo_contract_ref`.

Resolve each protected `player_ref` immediately before change and re-read
all matching `guild_members`, `guild_ranks`, and `player_data`. Multiple current
memberships are a conflict; they must never be collapsed into one evidence row.
Any drift from the authority fingerprint aborts that row; target state wins. Apply only the plan's
named player rows in one targeted transaction. Do not update `guilds`,
`associations`, `guild_ranks`, aggregate prestige, unrelated players, or raw
legacy guild-status bits. Use the supported runtime representation for a
current non-administrative rank rather than reconstructing a legacy bitset.

During clone rehearsal:

1. Begin the transaction, lock only the resolved affected player/membership
   rows, repeat the authority checks, and apply the planned inserts/updates.
2. Run both repositories' acceptance checks while the transaction is visible
   to the rehearsal session. Capture affected before/after values and prove
   the pre-recorded unaffected player/guild/association fingerprint is
   unchanged.
3. Roll back. Recompute all fingerprints and prove the complete clone returned
   to its pre-rehearsal state. Archive the rollback evidence.
4. Repeat the planner against the simulated post-apply snapshot. Every prior
   action must become `keep_current_authority`; no second insert or rank change
   is allowed.

Only after that rollback proof and cross-repository acceptance may a separately
approved production transaction be considered. Recompute authority immediately
before commit, keep the transaction targeted, capture commit evidence, and
retain the tested rollback procedure. A mismatch, unresolved authority,
missing support disposition, or post-check failure means rollback and stop.

## Permanent non-restoration

`conflicted`, `obsolete`, and `insufficient_evidence` rows remain reset. Their
disposition evidence must state why semantic restoration was impossible and,
for `player_support`, the private case/reference and required future proof.
Support may add new semantic evidence and produce a new reviewed plan; it may
not bypass the classifier, reuse a numeric ID, or grant a legacy administrative
role.

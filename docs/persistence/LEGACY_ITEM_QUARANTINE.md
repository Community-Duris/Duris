# Legacy Item Quarantine Classification and Recovery

The target-wins import correctly left ambiguous player and locker payloads outside current
item authority. Quarantine is preservation, not proof that a row is safe to activate.
`scripts/classify_legacy_item_quarantine.py` classifies a protected frozen-stage evidence
packet and can emit a recovery plan; it does not query descriptions, mutate a database,
or execute recovery DML.

## Freeze and build the evidence packet

Restore a fresh production backup and the retained frozen import stage into an isolated
host. Stop every writer and take one repeatable-read snapshot. The evidence builder must
join every one of the 38,257 quarantined `player_items` and `locker_items` rows against:

- the original source row and its parent row;
- accepted player/locker/chest owner identity;
- every occurrence of the numeric UID in stage payload and live authority;
- the active object prototype set and current artifact vnums;
- canonical item affects and extra-description rows; and
- the live allocator value and maximum retained/live UID.

Create a random per-run secret in an owner-only directory. Use domain-separated
HMAC-SHA-256 references for each source row and owner; do not publish raw UIDs, owners,
descriptions, or unsalted low-entropy hashes. `metadata_fingerprint` must cover the exact
payload plus its ordered metadata rows. Record how many indistinguishable metadata and UID
candidates exist rather than selecting the first.

The strict evidence JSON has top-level fields `version` (1), `allocator_next_uid`,
`live_uid_floor`, and `rows`. Every row contains:

```json
{
  "row_ref": "<64 lowercase hex>",
  "source_table": "player_items",
  "source_row_id": 1,
  "item_uid": 1,
  "parent_ref": null,
  "owner_ref": "<64 lowercase hex>",
  "owner_proven": true,
  "vnum": 1,
  "prototype_state": "current",
  "metadata_fingerprint": "<64 lowercase hex>",
  "metadata_candidates": 1,
  "uid_candidates": 1,
  "live_uid_conflict": false
}
```

`source_table` is `player_items` or `locker_items`. `prototype_state` is `current`,
`missing`, or `artifact`. `parent_ref` points to the exact row, never just a colliding
numeric UID. `owner_proven` is true only when the active player or locker/chest identity
is unique and accepted.

## Mutually exclusive classification

Run without a recovery plan first:

```bash
python3 scripts/classify_legacy_item_quarantine.py \
  --evidence /private/legacy-items/evidence.json
```

Routine output contains aggregate counts only. Every row receives exactly one primary
class in this precedence order:

1. `artifact`
2. `conflicted_owner`
3. `unknown_prototype`
4. `uid_collision`
5. `insufficient_metadata`
6. `missing_ancestor`
7. `cross_owner`
8. `cycle`
9. `depth_exceeded`
10. `dependent_ancestry`
11. `recoverable`

The classifier retains overlapping reasons internally but counts only the primary reason.
`dependent_children` therefore measures rows whose own evidence is sound but whose full
container chain reaches a rejected row. The command rejects the evidence before reading
dispositions or planning unless its aggregate `rows` total is exactly 38,257.

A row is `recoverable` only when owner, prototype, non-artifact status, exact metadata,
numeric identity, and every same-owner ancestor are unique and the complete graph is
acyclic and within the runtime depth bound. A numeric UID collision alone never supplies
owner evidence.

## Explicit dispositions and recovery plan

Artifact, conflicted-owner, cross-owner, cyclic, insufficient-evidence, and every other
unsafe row needs an operator disposition. The owner-only disposition record has version 1
and rows containing `row_ref`, `decision`, and an HMAC `evidence_ref`. Decisions are:

- `hold`: retain the quarantine without activation;
- `discard`: record the approved final disposition without deleting anything in this
  planning step; or
- `recover_new_uid`: allowed only when `uid_collision` is the row's sole rejection and
  the evidence proves either multiple UID candidates or a live UID conflict, while
  independent owner/prototype/metadata/ancestry evidence is otherwise complete; or
- `recover_descendant`: allowed only when `dependent_ancestry` is the row's sole rejection,
  the row's own evidence is sound, and every ancestor is explicitly or automatically
  approved for recovery.

No default means discard. A held parent also requires explicit hold/discard decisions for
every dependent child; a descendant cannot be recovered through a held ancestor. Generate
an owner-only plan only after all unsafe rows are decided:

```bash
python3 scripts/classify_legacy_item_quarantine.py \
  --evidence /private/legacy-items/evidence.json \
  --dispositions /private/legacy-items/dispositions.json \
  --recovery-plan /private/legacy-items/recovery-plan.json
```

The plan binds the evidence and disposition SHA-256 values. Collision replacements are
allocated monotonically from the maximum of the live allocator, live UID floor, and every
retained evidence UID plus one. `live_uid_floor` is the first unused value strictly above
every live or retained UID, and `allocator_next_uid` is the next available allocator value;
planning blocks if no representable replacement and subsequent allocator value remain.
Replacement allocation is deterministic by source table and source row ID. Recovered
descendant parent/root identities are rewritten through the same mapping. The plan file is
mode `0600`; stdout remains aggregate-only.

## Clone rehearsal and transaction contract

Never execute a plan directly against production. Restore a new production clone plus the
same frozen stage, validate both digests, and rehearse each approved recovery set as one
transaction. A set is one complete owner/root graph and must atomically:

1. lock the allocator, owner revision, target payload rows, stage source/metadata rows,
   quarantine rows, and every parent/root identity in deterministic order;
2. revalidate owner, prototype, artifact, payload, metadata, ancestry, plan, and allocator
   evidence against their protected fingerprints;
3. allocate only the planned new UIDs and advance `item_uid_allocator.next_uid` past the
   largest allocation;
4. write or update the exact payload and metadata rows, then establish planned parent/root
   identities;
5. insert matching `item_current_owner` and `item_ownership_baseline` rows and advance the
   exact `item_owner_revision` once for the coherent set;
6. mark only the corresponding quarantine evidence repaired after every durable row is
   present; and
7. use row-count/duplicate-key guards so any mismatch aborts before commit.

Capture byte-level before/after fingerprints for all unaffected payload, metadata,
authority, baseline, ledger, quarantine, artifact, and allocator rows. Already active and
already imported rows must remain byte-for-byte unchanged. Then run UID uniqueness,
topology, artifact authority, allocator floor, materialization, item ownership, runtime
compatibility, and every declared foreign-key check. Exercise login/materialization for
each affected player on the clone without printing identities.

Production recovery additionally requires explicit owner authorization, a fresh validated
backup, complete writer quiescence, exact plan and stage digests, captured rollback
evidence, and a player remediation policy covering notification, withheld/discarded items,
and compensation. Restore the exact backup on any discrepancy; never blanket-copy
payloads, regenerate every UID, infer ownership from a collision, weaken artifact
authority, or disable foreign keys.

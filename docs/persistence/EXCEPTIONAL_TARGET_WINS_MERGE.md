# Exceptional Target-Wins Account Merge

`scripts/import_legacy_dump.py` is a replacement importer. It is not authorized to merge
rows into an existing database. This document covers the separate, exceptional workflow
an owner must approve before any target-wins merge can attach source characters to a
pre-existing account.

## Identity boundary

Matching normalized account names do not prove matching owners. Password bytes, email,
creation time, and IP history are evidence, not automatic identity rules. In particular,
a shared historical IP is never sufficient. A colliding parent has exactly four safe
dispositions:

- byte-identical authentication metadata permits the existing parent;
- protected owner evidence may explicitly record `same_owner`;
- every proposed child and its dependent graph is marked `quarantine`; or
- every proposed child and its dependent graph is remapped to an explicitly recorded,
  non-colliding target.

Anything else is an unverified collision and blocks the merge even when every child row
is otherwise structurally valid.

## Protected preflight artifacts

Generate the plan on the protected host from the frozen source and target snapshots. Do
not put account names, character names, credentials, email addresses, IPs, database IDs,
or unsalted low-entropy hashes in the plan. Create one random per-run secret and use
HMAC-SHA-256 with domain separation to derive:

- `account_ref` from the normalized account name;
- `child_ref` from the source character identity; and
- password, email, and creation-time fingerprints from their exact source bytes.

The owner-only plan has this strict shape:

```json
{
  "version": 1,
  "source_accounts": [
    {
      "account_ref": "<64 lowercase hex>",
      "password_fingerprint": "<64 lowercase hex>",
      "email_fingerprint": "<64 lowercase hex>",
      "created_fingerprint": "<64 lowercase hex>"
    }
  ],
  "target_accounts": [],
  "children": [
    {
      "child_ref": "<64 lowercase hex>",
      "account_ref": "<64 lowercase hex>",
      "action": "attach",
      "target_ref": "<account_ref or null>"
    }
  ]
}
```

Use `action=attach` only for a new, byte-identical, or owner-approved parent. Use
`action=quarantine` with a null target, or `action=remap` with the approved target.

A semantic owner decision belongs in a separate owner-only record. `evidence_ref` is the
HMAC reference for the private adjudication packet, not an IP address or a prose claim:

```json
{
  "version": 1,
  "decisions": [
    {
      "account_ref": "<64 lowercase hex>",
      "decision": "same_owner",
      "evidence_ref": "<64 lowercase hex>",
      "target_ref": null
    }
  ]
}
```

`decision` may be `same_owner`, `quarantine`, or `remap`. Remap alone requires a
non-colliding `target_ref`. Run the verifier from an owner-only directory:

```bash
install -d -m 0700 /private/target-wins-preflight
python3 scripts/verify_target_wins_account_merge.py \
  --plan /private/target-wins-preflight/plan.json \
  --dispositions /private/target-wins-preflight/dispositions.json \
  --receipt /private/target-wins-preflight/receipt.json
```

The command prints counts only. It writes a mode-`0600` receipt binding the complete plan
and disposition digests only when every child follows an allowed disposition. The
exceptional merge executor must verify those digests again inside its deployment boundary
and durably record the receipt digest. A receipt is not authorization for a different
plan, source snapshot, target snapshot, or retry.

## Existing-collision repair

For an already attached collision, owner adjudication still comes first. If common
ownership is not established, prefer quarantine until a remap owner is proven. Rehearse
on a fresh production clone and generate a separate protected manifest containing the
exact account row and every affected mapping, with each mapping's player rows and
aggregate dependent-row counts. Do not expose that manifest in a ticket or log.

The quarantine transaction must:

1. require the verified plan/disposition receipt, a fresh validated backup, exact target
   confirmation, and zero application/writer connections;
2. lock the one account, manifested mappings, players, and dependent ownership roots;
3. verify the target password, email, and creation bytes still match the preflight
   fingerprints;
4. block the colliding account, block the manifested mappings, and make only the
   manifested source players inactive so the complete account-/character-scoped graph is
   inaccessible without deleting or rewriting it;
5. require exact affected-row counts, validate all foreign keys and item ownership, and
   compare byte-level fingerprints for the target credentials and every unrelated
   account/player/dependent row before committing; and
6. emit aggregate-only reconciliation and health evidence.

This deliberately quarantines the pre-existing account as well as the imported children;
leaving account-scoped bank or history rows visible to the retained credentials is not a
complete quarantine. Restore the exact backup to roll back—do not improvise inverse DML.

A remap requires a separately verified destination owner and must move the manifested
account- and character-scoped graph atomically while preserving item ownership and every
foreign key. No checked-in tool automatically chooses between quarantine and remap.

Production execution requires explicit owner permission. After commit, reconcile account
selection, bank ownership, item custody, ship ownership, notification/read visibility,
and all foreign keys; start the service only after the runtime readiness gates pass and
review authorization/refusal logs through the soak period.

# Shop recovery forest bindings and producer preparation — 2026-10-05

The original shop publication holder keeps its full player checkpoint body in
`original_shop_body`, a volatile pipeline slot. Restoring the original critical
command and publication hold does not restore that body. Current SQL sidecars
contain current item values; they cannot independently recreate deleted original
values or original native list order. This is an implementation gap in the
existing shop recovery path, not a new release requirement.

## Integrated pure value contract

`src/economy/shop_trade_recovery_manifest.c/.h` now provide bounded canonical
forest binding codecs, registered once in the maintained production Makefile.
The implementation has no shop command, SQL, world, producer or allocator
dependency. It uses the existing item-list codec and SHA-256 implementation.

Each binding retains the original ordered UID list, canonical byte length and a
digest bound to the explicit recovery version, forest role, count and bytes.
Four required roles cover player/keeper BEFORE and AFTER. Two optional paired
roles cover the full live destination BEFORE/AFTER. A present empty forest has
its canonical empty-list digest; it is distinct from an absent optional witness.
Each list admits the existing 4096-item bound. Zero, duplicate and reserved
maximum UIDs, invalid or noncontiguous trees, noncanonical bytes, malformed
roles, lengths and trailing data refuse without changing outputs.

Exact integrated SHA-256:

- C: `20ed4076709855dc27cce60fea1c5f4f09c674bb0d8ce27ca29e2feb57cd47b7`
- H: `9174ca1fac0372a0f36037ac2150f84355da9d3acff74fdbf71e0192c8809db9`

The registry pins and generated matrix retain `source_integrated_unqualified`.
Neither a hash nor a decoded value grants custody, admission, publication or ACK
authority. No actual command version, gameplay caller, admission gate, legacy
behavior, journal limit or accounting activation changes in this milestone.

## Prepared command and producer consumers

The private next command version extends the accepted v7 weight/status contract
with these bindings. It preserves versions 1–7, refuses dropping recovery facts
through older encoders, stages decode output and stays within the existing
384 KiB command-payload ceiling. Six maximum UID lists use 192 KiB; the existing
128 KiB selected literal remains separate. Final encoded-size checks still apply.
This command consumer is not installed by the pure-codec milestone.

The private producer freezes the exact held original player and sealed keeper
forests plus their ordered prospective AFTER images before journal admission.
Shared keeper/destination transformations replace the publication duplicates.
Manifest assembly stays local until every transformation completes; incomplete
v8 fields never enter a v7 encoder. Keeper AFTER now meets the existing native
capture budget before admission. A complete physical player census counts
omitted NORENT bodies and refuses a purchase whose final physical count would
exceed the existing 4096-object publication limit.

A separate pure reconstruction helper resolves current physical parent IDs in
retained UID order and verifies complete membership, payload presence, roots,
slots and the canonical binding. Row IDs remain current graph edges, not
historical identity. The private live publication reader compares the original
held player binding and complete locked current BEFORE/AFTER player/keeper cuts.

Full live target literals can contain non-durable NORENT children omitted by
ordinary saves. Cold recovery must use the separate persisted player binding;
it must not recreate those children or require their presence to acknowledge an
otherwise proved persisted operation.

## Parallel NPC schema preparation

Private migration 0060 extends exactly the three existing owner-type CHECK
ranges from 1..11 to 1..12 for the corrected native-mobile owner. Exact original
or completed replacement checks are accepted for partial retry; divergent or
unenforced constraints refuse. Each DROP/ADD pair is one ALTER. No rows, columns,
indices, other constraints or historical baseline values are rewritten.
The separate checksummed manifest entry preserves the executable verifier mode.
Coherent 0057–0060 integration and actual engine fingerprints remain pending;
no earlier measured schema is relabeled as this new chain.

## Verification and remaining work

Independent persistence source review accepted the bounded value/producer
foundation after the keeper-budget, manifest-staging and physical-count fixes.
Changed-line formatting, archived preimages, exact raw pins and source checks
passed. The registry generator is an inventory check, not gameplay evidence.
Compilation, tests, database migration execution and native/persistence/recovery
journeys remain deferred to major-plan readiness as requested.

Original complete SQL preimage authentication, cold projection readers, startup
replay registration, actor/keeper rebind, physical publication and guarded ACK
remain unfinished. The command/producer, focused link recipes and pending
schema chain must be integrated coherently before qualification. Plans 2–4,
combined Plan 5 and R1–R8 release acceptance remain open; the pure codec does not
complete accounting or qualify a runtime writer.

Private evidence remains in `tmp/plan4-shop-recovery-manifest-proposal-20261005`,
`tmp/plan4-shop-recovery-producer-proposal-20261005` and
`tmp/plan3-native-mobile-owner-migration-proposal-20261005`. Accepted previous v7
and destination-weight source receipts are preserved.

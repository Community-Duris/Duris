# Plan 5: authoritative lifecycle receipt discovery request

Native/shared owner: Plans1–4 primary. Independent consumers: Plan5.
Read-source base `4e786c49ba239923de66467b585876208ed4c38e`, merged from
published primary `b21b0c28dec5a49ad1ebb98d965cdff5bc3f2e59`.
Native source tree remains `84fd8dc7329646d6a32760c1be53e0ddcea1a533`.
This is a narrow interface request, not an implemented native change or
executed native/lifecycle qualification.

## Required discovery invariant

The [retained receipt contract](LIFECYCLE_RETAINED_RECEIPT_PREPARATION_2026-10-04.md)
makes `economic-evidence/lifecycle-<original-operation-id>.elr` mandatory for
new completed lifecycle installations. A directory scan can validate surviving
files, but cannot establish which completely missing files must exist.

The current authority-bound catalog has baseline initialization state, original
initializing operation and opening key. It has no explicit initialization
participant/receipt-required discriminator. The native lifecycle owner uses
its request's original ID to recognize its own initialized transition on retry.
That caller already knows it is executing a lifecycle request; an independent
restore reader does not have that external request.

General private epoch append accepts a nonzero creating operation, and general
private baseline initialization accepts a nonzero operation without requiring it
to differ from that creating operation. Baseline preparation can also use that
same original ID. Thus operation-ID equality and transition kind1 alone do not
establish a disjoint, published lifecycle-origin rule. A valid general baseline
book must not become a lost-lifecycle finding merely because its IDs coincide.
No Plan5 reader will invent or silently infer that ownership convention.

## Minimal native decision requested

Provide an authority-bound per-retained-epoch initialization-origin field with
explicit values `legacy_unknown`, `baseline_participant`, `lifecycle_owner`,
or publish an existing equally authoritative disjoint predicate with proof of
the same-ID general-participant counterexample. Preferred new field:

`initialization_origin`: bounded enum owned by the private participant,
serialized in the epoch catalog and included in its complete encoded digest
bound by `authority.eal`.

Required invariants:

- Only a known initialized descriptor can declare a known origin. Known-never
  and legacy-unknown initialization do not create a completed lifecycle claim.
- Generic baseline initialization declares `baseline_participant`; equality
  with epoch creation/preparation IDs does not promote it to lifecycle origin.
- Lifecycle completion declares `lifecycle_owner` in the same authenticated
  staged bundle as its `.elr`, book/common receipt and selection. It is not a
  public caller-supplied assertion or a separate later marker write.
- A lifecycle origin requires the existing original
  `baseline_initializing_operation` to equal the receipt request operation and
  epoch creating operation, with the existing opening/epoch/source bindings.
  The required filename derives from that **already recorded** original ID;
  no duplicate operation-ID field is needed.
- Origin remains immutable across population, deactivation, later epochs,
  mapping renames/retirement and erasure of current aliases.
- Older descriptors remain origin-unknown unless an explicitly verified
  migration supplies original evidence. Missing `.elr` files must never be
  interpreted as generic origin. Unknown origin cannot earn complete lifecycle
  preservation qualification.

The native owner chooses format version/layout and compatibility implementation.
Plan5 does not independently edit shared catalog codecs, participant signatures,
staging/coordinator logic, migrations or activation owner. Content authentication
against coherent authoritative rewrites remains the external source/backup
generation gate; this request supplies required-file discovery, not that proof.

## Consumers and required tests

Consumers are the independent catalog/receipt readers, copied-candidate restore
preflight and post-replay qualification, bounded operator evidence reports,
backup-generation inventories and retention/export/erasure guards. They enumerate
every retained lifecycle-origin epoch, require exactly its original receipt,
validate all present receipts independently, and preserve unknown provenance.

Native and independent tests must meet on the same published source:

- General initialized book with coincident creation/initialization/preparation
  IDs remains generic and does not require an `.elr`.
- Completed lifecycle receipt loss refuses, including loss of its entire file
  before a directory scan sees anything, with epoch/control anchors retained.
- Old inactive lifecycle epochs still require their original receipt after
  population, deactivation/turnover and current mapping/alias changes.
- Missing/foreign/duplicate/origin-conflicting receipts refuse without repair.
- Participant state, `.elr`, book/common receipt and selection publish/recover
  atomically under the complete shared-journal fault and allocation matrix.
- Legacy origin stays unknown; neither absence nor a self-asserted sibling file
  supplies an origin migration or activation permission.

Plan5 will continue independent validation of present `.elr` envelopes, ordered
descriptors and exact baseline/common-receipt links while this native decision
is pending. That work cannot close complete required-file loss qualification.

## Central lifecycle inventory handoff

The primary also owns shared source registration/central lifecycle inventory.
The current `validate_data_lifecycle.py` required stores and
`migrations/data_lifecycle_manifest.json` omit the new file family. Register:

- ID: `file:economic-lifecycle-receipt`.
- Kind: `recovery_state`.
- Locator: `FLATFILE_ROOT/economic-evidence/lifecycle-*.elr`.
- Producer: `src/flatfile/flatfile_accounting_lifecycle_transaction.c`.
- Stable identity: original operation, lineage/epoch and lifetime keys.
- Private fields: ordered native wallet account aliases and bank names,
  original source descriptors and command capsules.

Retention/erasure must preserve required immutable receipt and baseline/source
bindings; current alias deletion cannot rewrite original snapshots or infer
them from mutable mappings. The native receipt digest includes private source
aliases, so redaction requires an explicitly designed authority/retention policy.
No ad hoc retention duration, alias policy or destructive cleanup is proposed.
Read-only operator output will remain aggregate and will not export aliases.

No activation, server boot, production mutation, audit correction or shared
file edit accompanies this request. R1–R8 and release remain open.

## Primary interface decision; source prepared, unqualified

The primary verified the same-ID general-participant counterexample and prepared
the minimal seven-file native change. Independent persistence source review
accepted it; it is not yet integrated or natively qualified. Plan5 can prepare
its independent consumer against this interface while native qualification and
the matching combined-candidate evidence remain pending.

`epochs.eae` catalog envelope version3 retains the160-byte row. Immediately after
the initialization-state byte at row offset96, row offset97 is the new origin:
`legacy_unknown=0`, `baseline_participant=1`, `lifecycle_owner=2`. The next six
bytes stay zero; the original initializing operation starts at offset104 and
the40-byte opening key at120. Versions1/2 decode origin unknown. Unrelated v3
rewrites preserve that unknown; they do not migrate or infer provenance.

Known origin requires initialized state. Lifecycle origin requires the epoch
creator and initializer to match and transition kind1; duplicate lifecycle
initialization IDs across retained epochs refuse. Generic initialization stamps
baseline origin even when all IDs coincide. A separate private lifecycle entry
stamps lifecycle origin in the existing book/catalog/control/baseline/common
receipt/selection/`.elr` atomic bundle. Conflicting initialized origin cannot be
adopted on retry. Neither origin selection nor migration is a public caller flag.

The lifecycle receipt remains canonical version1, with its existing original
request, epoch, initializer, opening and source bindings. Retained receipt retry
rejects known generic origin; historical unknown receipts can still be validated
without promotion. Complete required-file discovery must enumerate known
lifecycle origins and use their already-recorded original initializing ID.
Unknown initialized origin cannot earn complete lifecycle preservation proof.
The complete encoded catalog remains bound by `authority.eal.epochs_digest`.
Older readers reject v3; matching Plan5 reader integration is required before
qualifying a new-format candidate.

The prepared source also registers `file:economic-lifecycle-receipt` in both
central inventory owners, using their existing manifest fields. Protected
immutable aliases/source descriptors and original evidence remain retained;
disclosure stays pending. No new store, receipt format, schema migration,
retention policy, activation permission or additional release gate is added.

Primary private input receipt:
`tmp/lifecycle-initialization-origin-proposal-20261004/source-pins.local.json`.
Authority source73a1ea641390a4d0e1d3440aefa77ee63a3cbdb5671a100714b51bbfb6dba68d;
baseline source8b6dbff6d4249cad943c0c61134a703cff1c08934213415c3ee5c275f2687b13;
lifecycle source3ddf535b07305e8305b05f5ecdd8be5fb317bcd50683de3e14f1e3c42b7a8e51.
These are source-review inputs, not published production bytes or runtime proof.

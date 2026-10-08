# Immutable lifecycle retained receipt preparation — 2026-10-04

Status: source implemented, **UNQUALIFIED**. No compiler, tests, AST checks,
native engine, SQL service or gameplay journey ran. Qualification remains
deferred until the major plan is ready.

## Source-established missing requirement

The previous `flatfile_accounting_lifecycle_transaction::install` checked
`control.active_epoch` and returned `EALREADY` before an original-ID lifecycle
lookup. Its returned mappings existed only in caller memory. Baseline receipts
retained economic account identities and original witness/plan bindings, but
did not preserve the lifecycle's ordered full mapping snapshots. Rebuilding
that output from later mapping rows could substitute renamed aliases, later
revisions, or changed creating/retiring/last-operation fields.

BEFORE is preserved at
`tmp/lifecycle-retained-receipt-before-v1.local/manifest.json`, base
`862f00381d08263cd223a54deb87ccd958e853e5`, SHA-256
`25a08161463ea2e39969240ae8b6a521d2e667f84c3ecdad22bfcee745ce055c`.
It records 19 selected inputs, not a complete compiler dependency closure.
The failure is established by source; no executed RED is claimed.

## Receipt and original-ID retry

Only `src/flatfile/flatfile_accounting_lifecycle_transaction.c/.h` changes.
The owner stages `economic-evidence/lifecycle-<original-operation-id>.elr`
through the existing authenticated staging view in the same final authority
bundle as mappings, baseline witness/receipt/reservations and epoch selection.
The original-ID filename prevents a new lineage from silently shadowing an
existing lifecycle operation. No common storage codec, shared authority API,
migration or public request/result signature changes.

The new private v1 format has an eight-byte `DURELR` magic, a version, bounded
body length and SHA-256 checksum in the existing 48-byte envelope layout. Its
body retains the exact request (operation/lineage/epoch, actor, accepted time,
explicit coverage digest, boundary digest and original external assertions),
resolved coverage, opening account, actual original baseline revision,
lineage-creation identity, selected control revision and full immutable epoch
membership/initialization provenance. It also stores the original encoded
baseline command, canonical witness and command-bound plan, using the existing
native codecs for those types.

The frame also retains bounded original native wallet descriptors
(PID, private account alias, race context, native revision and exact balances)
and bank descriptors (private name, context, native revision and exact balances).
Decode recomputes the existing `holding_source_digest` and coverage algorithms,
binding those descriptors and their original order to the independently
retained baseline witness. Each positional original mapping must match its
descriptor and the witness's account, balance, revision and source digest.
A canonically re-encoded frame with a changed valid locator/PID therefore
cannot borrow unchanged baseline evidence. No live native recapture is used.

Every original mapping retains its original position and all fields: account
key, locator kind/native ID/name, creating/retiring/last-operation IDs and
revision. New native mappings legitimately have revision zero with matching
creating/last operation; that established authority contract is preserved.
No name, position, revision or historical operation is reconstructed from
mutable mapping rows on retry.

Mapping revision/creating/retiring/last-operation fields remain authoritative
immutable `.elr` metadata: the baseline does not independently attest those
fields. A checksum is encoding integrity, not authentication against arbitrary
valid rewrites of authoritative storage. This slice supplies neither an
authenticated external restore provenance capability nor reconstructed proof
from mutable authority rows.

The decoder checks bounded counts/names/blobs, canonical re-encoding, unique
account/native locator coverage, matching baseline holding identities and the
exact request/epoch/opening/boundary/coverage/command/plan bindings. Fresh
construction and caller output are fully allocated before durable commit;
only a statically verified nonthrowing receipt move remains after success.
Final staging retains unique destinations and the shared bundle byte/row limits.

Install resolves retained original-ID evidence before `EALREADY`, external
fresh-install gates or mutable native recapture. An exact request uses the
actual `flatfile_accounting_baseline_lookup` to validate the native common
receipt, witness, plan, book and opening reservations, and compares historical
epoch membership/initialization links. Caller output comes from the immutable
original ordered mapping snapshots. Later mutable mapping renames, retirement
or revision changes are not consulted. A changed original request/opening
conflicts; missing/corrupt completed original history refuses without fallback.
An initialized own transition without `.elr` is lost history, not authorization
to recapture or fabricate a receipt. Old installations lacking the new receipt
therefore fail closed; no implicit migration is supplied.

“No writes on exact retry” means no **new** afterimages, authority commit, native
capture/mutation, mapping update or epoch selection. Existing authenticated
shared-journal recovery under the exact authority lock may finish any previously
published authenticated authority bundle before lookup, including a bundle
other than this original operation. No exclusive original-operation recovery
scope is claimed. This API is consequently not described as literally read-only.
A later active epoch can coexist with valid historical
retry evidence; retry does not select the original epoch again.

## Deferred qualification and integration

`tmp/lifecycle-retained-receipt-prepared-v1/cases.md` is a declarative case plan,
not an executable runner. Its 32 original declarations are preserved;
`cases-source-binding-v2.md` adds six descriptor/cross-record declarations.
The prior insufficient source pins `aa3ceffe…`/`3ca731fe…` remain preserved in
`source-insufficient-v1/`. No one ran those counterexamples or claimed RED.
Qualification requires the actual lifecycle owner, sealed view,
native baseline lookup/codecs, authority catalog/common store and shared journal;
a success stub for retained lookup or reservations cannot qualify this slice.
Source formatting, raw hashes and whitespace/diff inspection are preparation,
not native tests. Both source BEFORE and AFTER need complete qualified compiler
closures at the major-plan gate; this preparation supplies selected inputs only.

Frozen-boundary and virgin-state booleans remain external assertions. Setting
them does not authenticate a native cutover boundary, complete writer/holding/
pending-command census or activation permission. Production activation remains
unwired and blocked on those independent proofs and full crash/restart testing.

The new `.elr` format is an explicit **Plan 5 handoff**. Ordered locator names
are private aliases, not durable economic identity. Independent backup,
restore, export, deletion/erasure and retention integration must include the
new immutable format before activation. No retention/governance rules are
invented here, no raw private aliases are recorded in this report, and this
source slice does not complete R8. Plan 5 owns the independent backup, restore,
export, erasure and retention consumers and evidence. Primary owns the native
producer/lifecycle format and shared source registry/central registration.
Current Plan 5 authority/baseline readers and `validate_data_lifecycle.py` do
not recognize mandatory `lifecycle-*.elr` evidence. That inventory/restore gap
must close before activation; no guessed retention policy is introduced here.

## Frozen source review checkpoint

Final raw source SHA-256:

- `flatfile_accounting_lifecycle_transaction.c`:
  `8fc334344c06df97a7fd245e1ad6ebf62c5394b42b3f6b29f1b6eeaf3f5ae254`.
- `flatfile_accounting_lifecycle_transaction.h`:
  `db99b59ccbaecf3b9cd3b1f22cc2430f5d44c579412abfceba288e0096b4c117`.

`cpp_modernization_architect` completed the entire strengthened source review
at these pins and reported no further source blocker for this preparatory
scope. Review checked descriptor/witness/account binding, original ordered
coverage, bounded canonical decode/re-encode, actual retained native lookup,
historical epoch links, fresh same-bundle staging and precommit output
construction/nonthrowing final transfer. Mapping operation/revision metadata
remains explicitly immutable-frame authority. This is source review, not
executed qualification or authenticated external restore proof.

`tmp/lifecycle-retained-receipt-prepared-v1/manifest-after-v3.json` records the
19 selected source/API inputs plus this report and two declarative case plans.
Raw copies are preserved under `source-after-v3/`; these 22 selected inputs are
not a complete native compiler closure. The preparation describes 38 cases
without an executable harness or runner. `clang-format-18` formatting and
`git diff --check` of the owned source/header were completed; no compiler,
AST, tests, native engine, services or gameplay checks were run.
The prior v2 report/manifest remains preserved; v3 corrects recovery-scope
wording without changing accepted source pins or the 38 case declarations.

# Authority-bound baseline initialization interface v2

Dated: 2026-10-04. Native owner: Plans1–4 primary/assigned marker implementation
worker. Independent readers, reconciliation and release evidence: Plan5 owner.
**Native slice implemented/source-reviewed; unqualified. Independent readers
pending; lifecycle composition now has an unqualified source prerequisite.** This is a narrow shared format
handoff, not activation authorization or proof of authentic source holdings.

## Established gap and canonical owner

A completely deleted initialized book before its first successful baseline receipt
cannot be discovered from surviving book files/receipts. A revision-zero head's
initialization ID/opening is otherwise only self-asserted. Retain initialization
in `epochs.eae`, whose complete encoded digest is bound by `authority.eal`.
Do not create a self-asserted sibling marker or infer history from namespace absence.

Keep `DURECE1\0` family magic and the existing 48-byte envelope. Version2 applies
only to the epoch catalog; other authority envelopes remain version1. The 24-byte
catalog header retains lineage16, LE count32 and zero reserved32. Each epoch
retains its existing96 bytes and appends64 bytes:

| Entry offset | Bytes | Field |
| --- | ---: | --- |
| 96 | 1 | baseline state: 0 legacy_unknown, 1 never_initialized, 2 initialized |
| 97 | 7 | zero reserved |
| 104 | 16 | original baseline initialization operation ID |
| 120 | 40 | exact canonical opening account key |

Unknown/never states require zero ID/key bytes. Initialized requires nonzero
original ID and a canonical valid kind9 opening key, matching lineage, nonzero
authority ID and exact context. Unknown states, reserved bits, malformed keys,
trailing bytes or inconsistent zeros refuse. Maximum4096-row file is655432 bytes,
below the2MiB bound. Authority control hashes the **entire encoded catalog**.

## Atomic initialization and retry

Only the private accounting storage participant may change never→initialized.
It verifies current control revision/lineage/retained epoch membership, preserves
all other epoch fields/markers, increments control revision, sets last operation
and stages updated catalog/control. Baseline initialization combines these with
book head and16 empty indexes: **19 economic after-images**, one shared-journal
transaction, caller output all-or-nothing. Public transaction APIs continue to
reject economic-evidence writes.

Retried initialization must match the original ID/key and verify head/all16indexes
without rewriting. Missing files under an initialized marker are corruption.
Revision-zero head last_operation equals the marker ID; populated heads retain
the later batch ID. Initialization ID is not the derived baseline-batch operation.
This record creates no holdings, postings, source claims, receipt or activation.

## Compatibility and independent qualification

Version1 rows decode as legacy_unknown. Keep structurally intact old history
readable with its missing provenance exposed; refuse fresh initialization/new
baseline preparation for unknown epochs until an explicit independently verified
migration supplies original evidence. Rewriting a catalog to v2 preserves old
rows as unknown; only a genuinely newly appended epoch is known-never. Never
infer initialization ID from epoch creation or a populated head's latest operation.

Version2 readers must be coordinated before deployment; old binaries fail closed.
Rollback must retain a v2-capable reader, never delete fields to regain acceptance.
Plan5 should independently enumerate initialized markers, including old epochs
and inactive selections, then compare the retained baseline namespace and head
initialization key/ID. A fully absent initialized book must refuse qualification.
Unknown history cannot earn complete-book-loss qualification merely because no
files exist. External witness/source attestation remains a separate acceptance gate.

The native lifecycle installer currently rereads on-disk state and rejects duplicate
staged authority destinations; it also initializes before appending a missing epoch.
The marker participant alone does not fix staged-state composition. That shared
lifecycle integration remains a separate Plan1 prerequisite; do not qualify the
full installer from standalone initialization tests.

Prepare and later run original-ID conflict, opening substitution, entire/partial
namespace loss, all19 journal publication/recovery faults, v1 compatibility,
retention/deactivation/epoch turnover, populated updates, OOM unchanged-output,
maximum catalog, stale digest and malformed reserved/state cases. Native and
independent-reader evidence must meet on the same combined candidate. No new
compiler/test/native/SQL runs have occurred for this contract.


## Later lifecycle integration checkpoint

Source2b4591c21 adds [private staged composition](FLATFILE_LIFECYCLE_COMPOSITION_PREPARATION_2026-10-04.md)
through mappings, epoch membership, marker initialization, baseline book/receipt and
selection without duplicate final destinations. Its external boundary assertion
is not native proof; full source coverage, authenticated boundary authority,
independent Plan5 readers and qualification remain open. The marker-only catalog
format above remains unchanged. Historical namespace/composition gap statements
explain the prerequisite; they no longer assert that no staged view exists.

# Plan5 current61 shared source-pin handoff

The current61 original focused batch executes286 methods with zero skips and
one failure: the source-provenance contract finds stale lifecycle hashes in the
primary-owned writer registry and its mirrored coverage matrix. All other285
methods pass. Native source is unchanged; no release or producer qualification
is inferred. Plan5 requests only the two exact existing pin replacements below.

## Exact source and shared ownership

Primary source: `7e7e85146c340c154a2c977225f31ff8fa54c7da`.
Plan5 merge/test source: `1ad6c17a688afe0cdfa4083d67f443283f7db229`, published on
`codex/accounting-plan5`. The merge preserves prior Plan5 history. Its owned
origin-test conflict is resolved to the exact published version-aware blob
`f95c89c77ef98752a233bd414fdc66a3ab5ad989`; all strict position controls stay.

Primary-owned files:
`docs/persistence/economy_accounting/writers.json` and
`docs/persistence/economy_accounting/writer_coverage_matrix.json`.
Their `candidate_worktree_evidence` objects currently match exactly. Only two
entries in their `candidate_worktree_evidence.source_pins` map are stale:

| Map key | Recorded SHA256 | Actual current SHA256 |
| --- | --- | --- |
| `scripts/validate_data_lifecycle.py` | `948868127e84196d4998159c58e9114a8bdb0eaead2495d0d9da0f86d5f904ca` | `34979216bb645038129e61e6ad44e3a213f20b25a5d834273d3998fab6095d68` |
| `migrations/data_lifecycle_manifest.json` | `b37efc35e5865e466a5623e11f1f43f587d63ffa3de25acfe769faec79508fbf` | `d6beaeffce264e49aaa351bae946d18c3d480026bf6a32a7ddde5c6a9b072136` |

Every other declared source pin matches its actual input. Existing registry
SHA256 is `681f178c64912ff5833b6955c529cd263bab5e1813bbabf8b54032de690c0bc8`;
matrix SHA256 is `c7657a858c711ac56fde9813a1a8aff224177e94fcb174d70dd577e3c0406d11`.

## Required narrow primary change

Replace just these two pin values with hashes measured from the actual final
published files. Preserve every other candidate field, base/source identity,
route row, evidence policy, mandatory method, activation guard and unqualified
status. Regenerate the matrix through its existing generator so the mirrored
candidate object remains exact. No interface, schema, new field or runtime
behavior change is requested. If the primary changes either source again,
measure its final bytes; do not reuse an obsolete hash from this report.

Consumers and invariants:

- `SplitEconomyActivationContract.test_source_provenance_distinguishes_candidate_from_published_source`
  requires both candidate objects to match and every recorded digest to equal
  the actual source bytes. Its real SHA256 assertion must remain.
- `generate_economy_writer_coverage.py` consumes registry provenance and emits
  the mirrored matrix. Its normal `--check` remains required.
- `source_integrated_unqualified`, incomplete coverage, `BLOCKED` playable
  release status and existing refusal policies remain unchanged.

## Actual red and diagnostic evidence

The original batch runs through
`python3 -u -B tmp/plan5/run-current61-checks.py pure` on read-only source in
pinned Linux image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
All286 original methods run in215.188795 process seconds; the source-provenance
assertion is the only failure. Raw log SHA256:
`806bc57870400e27912f4f9e88fd6c87c6cf385dcf2ee53be6080d4b1692a420`.
Normal accounting, matrix `--check` and offline runtime checks pass, while
`validate_economy_accounting.py --release` still exits1 with
`writer has no executable evidence`. Normal/matrix success does not override
this original source-provenance failure.

The diagnostic command
`python3 -u -B tmp/plan5/diagnose-current61-provenance.py` changes only the two
values in copied JSON metadata and executes all71 original coverage/audit
methods. Each original source, test body and assertion is retained. It passes
with zero skips, failures or errors in14.909288 seconds. Removing those two
replacements recovers each original JSON object exactly. Both actual maintained
JSON files remain byte-identical, with the hashes above. This establishes the
specific proposed repair; the shared published provenance contract is still
failing until the primary applies it and reruns the original cases.

Evidence root:
`D:\CodexEvidence\accounting-plan5\bin\current61-7e7e-20261005`.
Original results/source maps/logs are in `checks/pure`; proposed JSON copies,
exact fields, unchanged source hashes, original71-method log and hash inventory
are in `provenance-diagnostic`. The exact request is also retained in
`tmp/plan5/current61-stale-source-pins.json`.

For the primary's curator: retain this original failed gate, the diagnostic
scope and the eventual actual pin repair/tested source. Current61 native audit,
genuine retained0056 upgrade/replay and maintained build qualification are
separate ongoing or completed component evidence; this metadata diagnostic
does not qualify full capture, real writers, R1–R8, release or activation.

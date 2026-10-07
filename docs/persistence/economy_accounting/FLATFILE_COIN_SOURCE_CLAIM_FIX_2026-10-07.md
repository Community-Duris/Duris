# Flatfile coin source claims - 2026-10-07

The typed flatfile COIN owner recorded its original source event but did not stage
the existing source claim with the native transaction. Successful retained replay
also did not authenticate that claim. This left the original R1 atomic-commit and
R3 durable source-event proof incomplete.

The owner now stages the original claim in the same authority transaction as the
wallet, UID-keyed pile, native ownership, references and accounting receipt.
Successful replay, post-commit verification and borrowed-lock retained proof
verify it. A missing, conflicting or corrupt retained claim is unavailable proof
and keeps replay retryable without changing durable bytes. New-operation staging
retains the original duplicate-source rejection. Rejected roots keep their
existing no-effect semantics. No claim format, backfill or fabricated history is
introduced.

The original regression fixture verifies a claim created by the real writer,
then clones that state and removes or damages only the claim. Both apply and
retained verification refuse those copies with exact durable-state equality.
The runner also links three genuine pure codec providers required by the current
command closure; it preserves the original flags, assertions and runtime budget.

## Qualification

This issue and the separately committed borrowed-lock room-pile reader were
qualified together on one eight-file candidate. The intermediate source-claim
commit is not represented as a separately executed complete native tree.

- The complete original 44-TU strict C++20 ASan/UBSan recipe passes: compile
  197.933 seconds; original suite 3.025 seconds within its unchanged 120-second
  limit. Original interrupted-commit, stale/replay and final split-child cases
  remain and run. The same execution reaches all 30 new reader groups.
- Both original 754-provider production builds and fresh full links pass within
  their unchanged 900-second budgets: SQL/Maria 61.55 seconds with 34 rebuilt and
  720 authenticated reused providers; flatfile 68.92 seconds with 36/718.
- Primary independently authenticates 1,336 component source members, 734 native
  component artifacts, 6,434 production source members and both complete
  1,510-member native cache exports, including actual objects, dependency files
  and original compiler/link arguments.
- Seven changed-line formatting transformations preserve every C++ token and
  quoted literal of the sanitizer-qualified source. All 926 writer policies,
  backend evidence states and lexical census entries remain unchanged.
- Original shared contracts pass. Two native contract attempts hit missing WSL
  libm/libmvec libraries; their unchanged recipes pass on the original private
  QA Docker image. The failed attempts remain recorded. The writer provenance
  contract passes after refreshing only the changed source hashes.

Evidence lives in `tmp/coin-flatfile-cold-proof-prerequisite-20261007/` and its
corresponding `bin/tests/` directory, with primary receipts under
`tmp/primary-runtime-review-20261007/`. Worker handoff SHA256 is
`730f9595da19fa2250feacb8ce8c52e7149dd0077b02095b2302135e58b27740`;
the 250-file evidence seal is
`deb148daac8096b15ee5070dd7ffe4a2b00c9980a18b9376221134e0dfa3b82a`.
The genuine sanitizer ELF is
`5da1da62b162d4504ce3cf5e3bd1520aaf170adc55795d808e290b5e65795ecf`.
Prior fixture/compilation failures and owned work volumes are retained.

## Remaining acceptance

This closes the source-claim defect. It does not qualify flatfile cold boot,
shared publication/holds/ACK, central activation, every coin producer, complete
Plans 2-4, R1-R8 or release. Inactive behavior, the declined spell path and safety
gates are preserved. `coverage_complete=false` and `release=BLOCKED` remain.

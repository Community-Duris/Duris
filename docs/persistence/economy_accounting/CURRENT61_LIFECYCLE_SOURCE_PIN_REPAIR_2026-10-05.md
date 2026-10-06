# Current61 lifecycle source provenance repair

The original writer provenance assertion failed on the maintained source at
`7e7e85146c340c154a2c977225f31ff8fa54c7da`: the registry and its mirrored matrix
still recorded two predecessor lifecycle hashes. The published Plan5 handoff
at `75349a0c7012251cdbbe6d72bf0cf7e09fe2dd69` identified the same mismatch.
The primary reproduced the original assertion through its normal script entry
point before changing either file; it failed on the validator's actual hash.

Only these existing `candidate_worktree_evidence.source_pins` values change:

| Input | Measured current SHA256 |
| --- | --- |
| `scripts/validate_data_lifecycle.py` | `34979216bb645038129e61e6ad44e3a213f20b25a5d834273d3998fab6095d68` |
| `migrations/data_lifecycle_manifest.json` | `d6beaeffce264e49aaa351bae946d18c3d480026bf6a32a7ddde5c6a9b072136` |

The matrix was regenerated through `generate_economy_writer_coverage.py`.
Restoring those two previous values recovers both original JSON objects exactly;
all other candidate metadata, route rows, evidence, identities, policies and
qualification statuses remain unchanged. No lifecycle implementation changed.

## Verification on the actual maintained worktree

Executed in the existing `duris-474-build` WSL environment, without services:

```text
python3 -B tests/async/test_economy_writer_coverage_contract.py
python3 -B scripts/generate_economy_writer_coverage.py --check
python3 -B scripts/validate_economy_accounting.py
git diff --check
```

All 55 original writer coverage/activation methods pass, with no skips or
failures, in 87.141 seconds. This includes the original source-byte SHA256
assertion, matching candidate objects and the existing incomplete-coverage and
blocked-release assertions. Matrix `--check`, normal accounting validation and
diff whitespace checks pass. The validator reports 14 fixtures, 899 routes
and 2871 candidate occurrences; these are contract/inventory results, not
executable writer qualification. The initial module-style test invocation
failed to import its `_paths` helper; the supported direct-script invocation
then reproduced the actual red assertion before the repair.

`source_integrated_unqualified`, `coverage_complete=False`,
`playable_release_status=BLOCKED` and inactive accounting/activation gates are
preserved. This solves the stale provenance issue only. Full R1–R8, Plans1–5,
native writer journeys, recovery, reconciliation and release remain open.

Related evidence: the published
`PLAN5_CURRENT61_SOURCE_PIN_HANDOFF_2026-10-05.md` on `codex/accounting-plan5`,
and [current private native qualification](NATIVE_QUEST_PRIVATE_MAJOR_QUALIFICATION_2026-10-05.md).

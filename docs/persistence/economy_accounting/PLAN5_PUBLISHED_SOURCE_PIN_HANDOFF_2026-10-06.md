# Plan 5 published source pin handoff — 2026-10-06

Resolved by exact primary `43c72921404c80fcf15de1eeede3db816fca238e`, imported
at `a674246ad89fde0850e57191fe7ebd76acf9e8bb`. All 58 original contracts pass
with zero skips and all 153 pins match actual bytes. The original failed source
and reproduction below remain historical evidence. [The exact follow-up](PLAN5_RESTORE_PROVIDER_COMPOSITION_QUALIFICATION_2026-10-06.md)
records ownership, current source, commands, evidence and remaining release gates.

The current published writer registry has five source pins for CRLF working-copy
bytes. The corresponding published Git blobs have LF bytes. The original
coverage contract therefore fails on an exact published-source checkout. This
is a shared registry handoff to primary; Plan 5 has not rewritten the registry,
matrix, contract assertion, native code or source files to make it pass.

## Exact candidate and reproduced defect

Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Branch and remote destination: `codex/accounting-plan5`.
Import parent: `4b86e892cae1fc9e0eefc29dc52e2549706e4460`.
Imported primary: `8795a0b086a7f581c6c4b8080bc83245646fbf5d`.
Import result: `4b89f6116edeb12f7fe3e324ee9a3e94c1e2d7d3`.
Native tree: `03a97173396f720857b1ad58a2ab69b7859a2387`.
Migration tree: `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`, canonical 61.

All production, migration and script blobs match that primary exactly. The
merge is conflict-free and preserves all seven earlier Plan 5 branch tips and
the three separately committed native-recipe fixes. No other branch, force
push, reset or rebase is used.

On Windows Python 3.12.10, from this worktree:

```powershell
$env:PYTHONPATH='tests/async'
python -B -m unittest -q test_economic_sql_lifecycle_activation_contract test_economy_writer_coverage_contract
python -B scripts/validate_economy_accounting.py
python -B scripts/generate_economy_writer_coverage.py --check
```

The combined original test invocation runs 58 methods, with one failure at
`test_source_provenance_distinguishes_candidate_from_published_source`, first
exposing `src/mob/mobconv.c`. The other 57 methods pass, with zero skips. The
normal validator passes 14 golden fixtures and 920 routes. The matrix check
passes 2,876 occurrences / 2,818 unique sites, with zero unmapped sites and
`coverage_complete=false`, release `BLOCKED`. Those passes do not waive the
provenance failure.

An initial package-qualified unittest invocation ran the three activation
methods but failed to import the coverage module because `_paths` was absent
from its Python search path. The explicit original test-directory search path
above fixes only that invocation issue and exposes the actual provenance defect.

The independent receipt `tmp/plan5/equipment-v2-import-proof.json` checks all
153 registry pins. The five mismatching worktree files equal the raw bytes from
`git show 8795a0b086a7f581c6c4b8080bc83245646fbf5d:<path>` exactly. For each,
converting only LF to CRLF produces the current expected hash. This establishes
a published-source representation defect rather than a native-code disagreement.

| Path | Current expected CRLF SHA256 | Published Git blob LF SHA256 |
| --- | --- | --- |
| `src/mob/mobconv.c` | `19c00d1c4bce0e2cd72cb5d0e9834896213a697ce5958f3b416edc3d2a851947` | `b79cc8825a40b6c6924c835181ab9bfb1ec96ce051da30ffd9f4922878e3312a` |
| `src/world/handler.c` | `cf91bd280c0711069f8eb6117addd66ff5aacda9afdeaf0fda97b21d1d1c73b4` | `3f046194166dbbfd949f9ec0199e72f6155283edbdac69a7fd186da21333ce07` |
| `src/world/handler.h` | `aac9a9f14b04ad0b449f00a36cb3e24e75cb1ee61abd4bd392db642e98850504` | `69cf721db5b448bd67b76074a5af2cb338d46ef3425dc8cae7bb18bc7803f124` |
| `src/persistence/economic_sql_accounting_lifecycle_transaction.h` | `9db963588ebb5af1051e5381e443776bc9e96fd49eedcbb15ad0c8a0813b374f` | `072cab4bb68499082691321def13aed276529a78dba2a5d13312aed0afffa5b2` |
| `tests/async/test_economic_sql_lifecycle_activation_contract.py` | `1be30c4c5051ec295cf8b8d9cc0326646ed4b6792e8b9cd3b6405a6895c275fd` | `10da6a9517092507a6ed6255536fe8b6a09c85383efffe75edb0c1d22d0209c4` |

## Narrow shared request

Primary owns `writers.json`, `writer_coverage_matrix.json`, their generator and
the coverage contract. Bind only these five
`candidate_worktree_evidence.source_pins[path]` values to the published Git
blobs and regenerate the matrix using the existing generator. Its copied
`candidate_worktree_evidence` must agree exactly with the registry. Preserve
every other source pin, route, backend status, evidence requirement, refusal
policy and incomplete-release marker. There is no public interface, schema,
migration or serialized accounting change requested.

Consumers are the original coverage provenance assertion, the matrix generator
and release evidence reviewers. Proof should retain all 55 original coverage
methods, all three activation contract methods, the normal 14-golden validator,
matrix `--check`, and an independent comparison of all 153 raw pins against the
published source. Do not normalize the assertion or rewrite unrelated source
files to accept a hash from a different byte representation.

The source capture qualification can proceed independently. This defect remains
an unresolved shared provenance gate until primary publishes the exact repair.
Full R6 opening, independent activation verification, R7/R8, supported writer
journeys and release-host budgets remain open. Accounting remains inactive;
wallet-root exclusions and the declined inactive spell-path change are preserved.

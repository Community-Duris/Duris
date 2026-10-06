# Primary handoff: executable mode of canonical 0062 verifier

Fresh Plan 5 SQL qualification cannot execute the published verifier on either
private engine. This is established on imported primary
`8030e71b55286d77b751fe41044d978dd62fa4cc` and remains in latest primary
`707b9cdd3d2f72307e70f1bd8dbf3e1ab2674d29`.

## Exact requested shared change

Path: `migrations/immutable/0062_economic_pending_claim_consumption.sh`.
Published Git mode: `100644`. Required mode: `100755`, matching the previous
`0061_economic_baseline_equipment.sh` and the runner's direct-execution contract.
Current blob: `15d11ef526cc6748acbdd4c1a803e588ab5d3361`.
Raw Git archive mode: non-executable `0664`, restored exactly by the launcher.

The primary should publish a mode-only correction, for example by staging
`git update-index --chmod=+x migrations/immutable/0062_economic_pending_claim_consumption.sh`
in its own candidate. No file bytes, immutable SQL, manifest checksums,
data/wire fields, column types, constraints or schema fingerprints change.
No historical migration or authority is rewritten. Plan 5 has not changed this
shared path or applied an internal permission workaround to claim a pass.

## Invariant, consumers and tests

Every registered verifier must execute from an unmodified raw checkout or
authenticated Git archive. `scripts/migration_runner.py:553` calls
`subprocess.run([str(migration.verify_path)], ...)` directly. It does not call
`bash`. The canonical manifest sequence62 `verify` field names the path above.

Consumers include fresh migration, populated-fork qualification and Plan 5
restore, original-capsule audit, native baseline and managed retention recipes
using `migration_runner.run_pending`. The new shell verifier is self-contained;
it does not directly execute a separate Python verifier. No second executable
mode request is inferred.

The original full command
`python3 -u -B tests/async/test_restore_economic_coin_effects.py`, with native
cache off and explicit private canonical integration enabled, fails on both
MariaDB and MySQL after the owned head repair:

```text
PermissionError: [Errno 13] Permission denied: '/workspace/migrations/immutable/0062_economic_pending_claim_consumption.sh'
```

Fresh SQL/flatfile fixtures and native controls pass before this failure. The
method exits1 in 277.700380 seconds within the unchanged 1,800-second outer
deadline. Full terminal log:
`D:/CodexEvidence/accounting-plan5/bin/canonical62-recipe-head-green-01-20261006/original-canonical-restore.log`.
Log SHA256:
`af5a6f2b2f5544f63b363f3c3ce689139f8f972da6291fc51e13145ce2471dce`.

After publication, use unmodified raw source/mode and rerun the original fresh
both-engine method, then full `test_economic_sql_canonical_audit.NativeCanonicalAuditTests`
and `test_native_sql_baseline_audit.py`. Original providers, assertions,
compiler/sanitizer flags, child deadlines, SELECT-only readers and unchanged
authority checks remain required. A mode-only correction does not qualify new
allocation semantics, canonical metadata or producer/recovery journeys. Retain
this failed observation.

[The recipe/build report](PLAN5_CANONICAL_0062_RECIPE_FOLLOWUP_2026-10-06.md)
provides exact source, branch/base/result, tools-image versions, commands,
separate fresh build passes and sealed evidence. This primary-owned correction
blocks these SQL gates, not independent Plan 5 work or the notebook workflow.

# Restore session fixture matches required baseline tables

The restore session regression previously answered `0` for every SQL query.
The production integrity reader requires exactly three InnoDB baseline evidence
tables even when the restored history contains no economic roots. Its metadata
query therefore refused with `restore_economic_baseline_source_mismatch` before
the success-path session cleanup assertion could run.

The fixture now answers `3` only for that exact table query and `0` for the
remaining empty-history checks. It requires SELECT-only calls and observes the
metadata query explicitly. The incomplete-history case still refuses before any
SQL. Production readers, restore policy and all original assertions remain.

## Evidence and scope

- Original failed suite: `bin/tests/plan1-maintained-reader-pure-20261005-0ae3d1c5474f/immutable_migration_runner`.
  All24 methods ran; the complete-history session case errored. Stderr SHA256
  `6c48da4920291a393a72a97356c583a15d11fccbfdb895290a9c17845ff2300d`.
- Corrected private fixture SHA256:
  `34a1f0fd6c4dba9e2fb33efaab3363176ed22af8bb02140f0d8ae42fef83c74f`.
  All24 original methods passed, no skips, 5.765860 process seconds.
  Stderr SHA256 `6316319db18803a50dae10a36ff659f47006e36053036f6aef4e9ca64413fdd8`.
- Retained Linux artifacts:
  `bin/tests/plan1-installed54-native-audit-linux-20261005-5a399eb0ecb4/bin/tests/restore-session-fixture-discovery-fixed`.
  A first temporary wrapper discovered zero tests and is not evidence of a pass.
  The corrected execution exited0; its first observer missed two multiline
  docstrings. `classification-correction.json` checks every original named
  method against that retained output without repeating tests or changing them.
- The complete24-case execution uses the separately pending EAB2/schema61
  candidate and its normal supported manifests. This commit stages only the
  fixture response correction against the published schema56 branch; its
  existing manifest/head assertions stay unchanged. The fix's SQL metadata
  response and session behavior are independent of those head assertions.

This is a component fixture repair. It establishes neither a populated restore
nor full accounting qualification. The production integrity reader remains
unchanged; R1–R8, complete capture, release and activation remain open.

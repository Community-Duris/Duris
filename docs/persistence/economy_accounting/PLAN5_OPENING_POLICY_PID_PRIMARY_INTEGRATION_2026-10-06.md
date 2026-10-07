# Independent opening-policy and beneficiary readers integrated — 2026-10-06

Plan5 established ten false accepts across the two SQL engines: a new opening's
policy marker could be removed, or its original mapping and beneficiary PID
could be changed together. The old readers compared the claim account and
amount without authenticating all original identity and policy facts.

The integrated readers authenticate the retained V2 lifecycle request and
installation/inbox receipt, recompute original ESD1/ESR1 PID/amount/revision
digests, and compare positive source slots. Zero-value claim mappings and
retired mappings retain their original identity checks. Historical NULL policy
is still unknown; current pickup values do not replace original witness facts.
All checks remain read-only and never repair evidence.

## Exact primary integration and checks

Four files are exact completed Plan5 blobs from
`e4ea41892063dbd553d61b3d44d9d0ebd1e05904`:
`scripts/economic_sql_audit_origins.py`, `scripts/economic_restore_evidence.py`,
`tests/async/test_economic_sql_audit_origins.py`, and
`tests/async/test_economic_sql_canonical_audit.py`. The latter also retains the
completed original-equipment fixture correction. Seven shared reader/native
fixture inputs already matched and remain unchanged, including the primary's
partial-allocation/pair-batch and exact-integer UID fixes.

Primary qualification executes all eight selected pure classes in the existing
origin, canonical and reconciliation suites: **209 methods pass, zero skips**,
in 134.469 seconds. All frozen regular inputs remain unchanged. This is a
selected pure batch; it does not claim native methods or budget opt-ins ran.

Evidence is retained under `tmp/plan5-opening-policy-pid-primary-20261006/`:

- `SOURCE-PINS.json`: `2c7a203ffc98e6d055ba40e0776dbe84489fc014fd2486a92397f8a1e7b5c72b`.
- `PURE-RESULT.json`: `ba713869d15eab23292c9645890798cb7fbfff599f56cb3784a384958c203050`.
- `pure-batch.log`: `03948e51a0963ebd73cb41de148dbd181ed63a207d44c685a1ef7cc552a6f67b`.
- `INSTALLATION.json`: `c6d9629e8235e270a41046f5d8917f47a60f05d086276884bb41a6c9c0e87c0b`.

## Independently completed native evidence and limits

The consumed Plan5 original-opening report is
`PLAN5_ORIGINAL_OPENING_QUALIFICATION_2026-10-06.md`, from the exact peer above,
SHA-256 `89dbbf9e7064c7c1e2e881bf41eecac1b59dc463dd4191109cea15104e470148`.
A copy is retained with the primary evidence. It reports six original native
lifecycle/opening/recovery programs on fresh canonical0062 MySQL and MariaDB,
ten repaired refusal controls, and 82 full focused methods with zero skips.
The original 43 real providers, sanitizer flags and assertions remain.
This is peer-attributed native evidence; external protected artifacts were not
locally re-inspected or represented as new primary executions.

Primary verified exact native tree `4abb609524a1f1682ea4c190f82d75003c4d679b`
and migration tree `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` match the peer.
This Python-only issue changes neither tree. The later combined reader and
opt-in reports remain separately scoped evidence; their modeled budgets do not
qualify mixed gameplay workloads.

The peer's full recovery audit still correctly refuses two missing producer
source claims. That repair belongs to the shared producer stream. The private
0063 startup/world candidate and captured opening digests require their own
coherent qualification. No Plan, R1–R8, complete coverage, activation or release
gate is promoted. Inactive behavior and the declined inactive spell path stay
unchanged; unrelated worktree edits are preserved.

# Published source provenance repair — 2026-10-06

Plan5 established that five registry source pins hashed Windows CRLF working
copies although the published source blobs contain LF. The original raw-byte
provenance assertion therefore failed on the published candidate. Primary
independently reproduces that exact failure on authenticated raw Git inputs,
then repairs only those five registry values and regenerates the matrix with
its existing generator. All other148 pins,920 routes, backend evidence/statuses,
historical provenance fields and incomplete-release markers remain identical.
The matrix changes only the copied five candidate source pins.

Affected paths: `src/mob/mobconv.c`, `src/world/handler.c`,
`src/world/handler.h`,
`src/persistence/economic_sql_accounting_lifecycle_transaction.h`, and
`tests/async/test_economic_sql_lifecycle_activation_contract.py`.
Three precise `text eol=lf` attributes cover the previously unpinned checkout
representations; both handler paths already have them. Primary refreshes only
the equivalent line endings of these five local copies, independently verifying
that every refreshed byte equals its existing published blob. No source-content,
contract assertion, serialized accounting or schema change occurs.

## Verification

The primary raw-source evidence snapshot starts at
`a9db59aebc9bdacd315f48e4a889e943b5e44044`. Git archive's checkout conversion
initially prevents raw-byte equality; that failed setup is retained separately.
The corrected snapshot authenticates every selected raw blob with its Git object
ID before materializing it. The original provenance method then reproduces the
actual failure at mobconv before metadata repair. After repair all58 original
coverage/activation methods, normal14-golden accounting validator and generated
matrix check pass with zero skips. Primary separately reruns all58 methods on
the actual maintained checkout after precise LF refresh: exit0,12.112s.
All153 pins equal both actual local bytes and existing published Git blobs.
Independent persistence review confirms the five mismatches and narrow repair.

Full commands, original red observation, snapshot authentication, all153 comparisons
and terminal log hashes remain in
`bin/tests/published-source-pin-primary-20261006-r2/results.json` and
`bin/tests/published-source-pin-primary-local-20261006/results.json`.
The earlier archive setup failure remains under the non-r2 evidence directory.

[Exact external handoff](https://github.com/Community-Duris/Duris/blob/e1f50802b80d5c788bd4909a169a41348fd7d5b7/docs/persistence/economy_accounting/PLAN5_PUBLISHED_SOURCE_PIN_HANDOFF_2026-10-06.md)
retains its older source scope. This solves published-source representation,
not runtime writer qualification. Coverage remains false and release BLOCKED;
R6 opening, original producer journeys, R7/R8 and release-host budgets remain
open. Accounting stays inactive and unrelated WIP remains preserved.

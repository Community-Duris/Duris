# Coin-payload reader fix: primary integration — 2026-10-06

The exporter now checks stored and mapping-projected coin byte limits before
fetching payloads. Both projections exclude ignored non-coin payloads. This
solves the source-read ordering defect established by Plan5's actual original
reader observations on MariaDB and MySQL; it preserves the original limits,
read-only transaction and snapshot format.

## Exact import and original regression registration

Primary imports the five-file slice byte-exactly from
`da2153d002a5e1e335818f615b2825fa5693d077` over primary
`d37138dee6361825db594bff2afb07091c773540`. The executable SHA256 pins and
peer's original RED/SQL/native component evidence remain in the unchanged
[peer report](PLAN5_COIN_PAYLOAD_SOURCE_BOUNDS_2026-10-05.md).

The sole primary manifest change raises the existing UID-scope row's
`minimum_cases` from9 to11. Every other regression field and the complete
integration manifest remain unchanged. Actual discovery executes both new
methods rather than accepting a reduced nine-case run:

- `UidScopeTests.test_coin_payload_bounds_refuse_before_mapping_payload_reads`
- `UidScopeTests.test_mapped_coin_payload_budget_counts_repeated_joined_bytes`

## Primary checks and evidence limits

Original command:
`python tests/run_test_entry.py tests/async/test_economic_sql_uid_scope.py unittest tmp/plan5-coin-source-bounds-11-cases-20261006.json 11`.
All11 cases PASS, zero skips, exit0. The retained JSON records both required
method identities and all original cases. The earlier nine-case/minimum11
refusal remains recorded separately and is not claimed as the reader defect RED.

Normal accounting validation, writer-matrix `--check` and runtime compatibility
validation PASS. Normal validation covers14 fixtures/899 routes/2871 sites;
matrix still reports16 connected schema2 gameplay routes,
`coverage_complete=False` and `release=BLOCKED`. Integration `--list` previously
passed as inventory only; no integration execution is implied.

Current maintained native tree is unchanged
`bf7a92a728ad9b5b813626462e56533f8ba39c97`, with migration tree
`2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`. Primary performed no native/SQL
rerun for this reader-only milestone. Peer's minimal modeled SQL fixtures and
171-method/native-codec results retain their stated scope; they do not prove
canonical migrations, native producer coverage or full accounting capture.

The new peer head `236d20d9b7af62bb2fc8061e76655c20df0bc86e` is fetched; its
separate full-reader qualification report has not yet been integrated here.
Unrelated local SHOP harness edits and the restore metadata report are preserved.
Original reset/cold producer work, full Plans1–5/R1–R8 qualification, gameplay,
recovery and release gates remain open. Accounting stays inactive; the declined
inactive spell-path change is untouched.

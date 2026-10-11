# Repair legacy publication fixtures — 2026-10-11

The spell fixture now extracts the complete original runtime-identity allocator,
admission and borrow implementation rather than only its map. It supplies the
real observer declaration and a game-thread predicate using its existing thread
identity. The original scenarios, assertions, sanitizers and timeouts remain.
The copyover source check follows the actual `apply_with_writer` owning body and
verifies public delegation, preserving its original ordering and drain checks.
Production code and the declined inactive spell path are unchanged.

Both repaired checks passed against current production source at 54fef6421 in
the existing GNU 13.3.0 Docker environment: spell compilation/execution retained
AddressSanitizer and UndefinedBehaviorSanitizer (7.595 seconds), and the static
copyover check passed (2.325 seconds). Independent finite source review passed.
Evidence: `tmp/held-legacy-fixture-checks-root-20261011/RESULTS.json`;
result SHA256: 6ee64575ec94b08abd3240d2cb468cff66ecf4002221821f45eaf76a3fb47957.

Both existing drivers were absent from the source-pin registry and test rows.
This repair adds their two actual evidence-input pins: 484 total, all 482 existing
pins preserved, none changed or removed. No executable registration, complete
inventory, gameplay or shutdown journey is claimed. The 931 writer policies,
2,911 census entries and runtime coverage remain unchanged.

The separate frozen HRT batch completed with both production links and its
typed SQL/flat policy fixture passing, but eight of nine regression entries
failed. Its original FAIL evidence is preserved. These two current-source checks
do not relabel that batch or qualify the remaining runner repairs, HRT recovery,
actual SQL durability, activation, restoration, host behavior or release.
Accounting remains inactive, admission CLOSED, coverage incomplete and release
BLOCKED. R1–R8 and the original unfinished plans remain open.

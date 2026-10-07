# Primary integration: independent SQL composite sweep

Date: 2026-10-06. Maintained parent: `de42a11c09ed9f822d120f9697aaac42765a023b`.
Completed peer slice: `a40ee8cc5`; exact source read from descendant
`ece7a6280e209a43636b7717f848892e4377f1a9`.

The root-only audit could miss a standalone baseline control or a reservation
whose parent root/witness had disappeared. The new explicit `--all-namespaces`
mode rotates bounded root, control and reservation pages using their existing
indexes. Each namespace retains its own ceiling and resumes from its lowest
key on the next pass, so later lower-key commits can be observed and a growing
root history cannot starve the other namespaces. Reads remain SELECT-only;
transactions roll back before protected local progress is published. Findings
are retained and never corrected by this command.

The complete three-file import includes its necessary predecessor decoder:
`scripts/economic_sql_canonical_audit.py`,
`scripts/economic_restore_evidence.py`, and
`tests/async/test_economic_sql_canonical_audit.py`. The independent specialist
review found no blocking regression. All35 maintained test methods survive;
34 method ASTs are exact. The remaining original native method gains page,
EXPLAIN, orphan, rollback and unchanged-inventory checks and requires refusal
of noninteger SQL storage even when JSON normalizes its values to integers.
Original canonical capture and both imported fixture helper ASTs remain exact.

The operator guide now includes the implemented root and composite progress
modes, bounds, unsigned ordering and explicit v1-to-v2 upgrade. Root-only mode
refuses an upgraded v2 checkpoint. Rollback to an older executable must preserve
the v2 checkpoint; operators must not relabel it as v1. No migration or database
conversion is involved. Source imports are exact peer bytes and source modes.

Primary Windows checks on these imported inputs:

- Canonical audit:64 discovered,59 passed, five explicit platform/native skips.
- Origin audit after shared decoder integration:52 discovered,49 passed,
  three explicit native integration skips.
- Mobile grammar:9 discovered,6 passed, three native opt-in skips.
- Child identity:23 discovered,21 passed, two native/budget opt-in skips.
- Normal accounting validation and whitespace checks pass; release stays false.

Protected primary results are under
`bin/tests/plan5-composite-primary-integration-20261006/`; import receipt and
exact preimages are under `tmp/plan5-composite-sweep-primary-integration-20261006/`.
The [peer qualification](PLAN5_SQL_COMPOSITE_SWEEP_QUALIFICATION_2026-10-06.md)
reports117 Linux methods with zero skips, including both-engine SELECT-only
namespace/EXPLAIN/CLI cases and original restore checks. Those results remain
peer-attributed to its frozen native/migration inputs. They do not qualify the
different private753-provider producer candidate or its cold recovery.

The deadline is cooperative; returned-row limits do not establish a hard
server-work bound. Historical namespace passes do not form one consistent
whole-store cut. Complete native holdings, producer coverage, player journeys,
recovery, activation and R8 release qualification remain open. This closes the
independent audit scheduling gap, not R7 or a whole implementation Plan.

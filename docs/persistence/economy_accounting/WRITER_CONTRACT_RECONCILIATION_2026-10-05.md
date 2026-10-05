# Writer coverage contract reconciliation — 2026-10-05

Plan5 executed71 source contracts on frozen cd89d4b02:68 passed and three
failed. [The peer report](PLAN5_NATIVE_COMPONENT_BUILD_AND_CONTRACT_HANDOFF_2026-10-05.md)
records exact commands/input pins and failures. Source review establishes:

- Checked placement has ten distinct physical sites, including the reviewed shop
  callback, rather than the stale nine-site expectation. Six actobj locations moved.
- Shared terminal inventory unloading still has exactly two operations and two
  backend owners per operation; old absolute positions moved. The separate
  restoreObjects placement anchor also moved and is reconciled in that contract.
- Candidate evidence explicitly records source_integrated_unqualified. The old
  generator inferred unpublished source from any candidate record; the contract
  required an obsolete dirty-worktree shape regardless of actual status.

The contracts now resolve exact reviewed function/operation identities and compare
the entire checked-site set with the exact ten-site owner map. Repeated lexical
records deduplicate only the same physical path/line; added/removed calls, changed
expressions, ambiguous source identities or wrong owners still refuse. Terminal
unloading retains exactly the original two-owner exception, with one owner for
other legacy file sites. Native operations and registry ownership are unchanged.

The generator handles the two existing provenance statuses explicitly and rejects
unknown statuses. Integrated evidence preserves its base_commit/scope/source_pins
shape; its contract hashes every original component file. Historical unpublished
evidence retains dirty_candidate/published_base_commit/scanned_source_tree_sha256
checks. No fake dirty/scanned fields are added. Integrated source remains expressly
unqualified, coverage_complete=False and release=BLOCKED. No schema2 gameplay,
posting, source reachability or executed evidence is promoted.

Two Python AST parses, all raw source pins, generated census and whitespace are
checked. These are source checks, not execution of the repaired unittest cases.
The requested major-plan test batch remains deferred; the71-test peer run is
historical failing evidence and does not qualify the repaired combined candidate.
Native builds/runtime/recovery and R1–R8 acceptance remain open. Inactive behavior
and the declined inactive spell path are unchanged.

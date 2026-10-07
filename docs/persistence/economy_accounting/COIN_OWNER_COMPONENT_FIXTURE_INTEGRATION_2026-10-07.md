# Coin owner component fixture integration - 2026-10-07

The maintained owner component fixture could not reproduce its restored physical
refusal cases: it lacked the real stopped save-pipeline provider closure and
registration, supplied a floor literal in equipment slot zero, omitted the four
canonical string-presence bits, and constructed a completion with revision zero.
The defect predates the published post-ACK room recovery milestone.

## Fix and scope

The fixture now prepares a private empty journal through the real stopped
pipeline, registers and reads back the original immutable held operation, checks
identical registration and changed-byte refusal, and shuts down after the
original assertions. The native literal uses the actual floor slot -1, all four
string bits, and the maximum committed endpoint revision. No worker or ownership
epoch starts. Native publication, covered-revision observation and guarded ACK
are abort-only boundaries, and none is reached.

The maintained runner links the complete real provider closure and preserves the
35 original owner cases in both SQL-header and flatfile profiles. Obsolete text
replacement anchors are removed because the current ACK fixture already contains
the stronger retry and physical-reverification assertions. The original
single-compile recipe, 300-second compilation and 30-second case budgets remain.
Private parallel/cache controllers are not imported.

The physical harness also retains its five previously qualified literal-shape
controls, so the maintained runner's 30-case list matches its actual harness.
Every original 25 physical control remains. This is a fixture integration;
production code, manifests, writer policies, inactive behavior and the declined
spell path are unchanged.

## Evidence

- Actual native owner execution: 35 cases per profile, **70 PASS**, including
  four real stopped-pipeline setup/cleanup receipts; no abort-only boundary call.
- Two fresh sanitizer harnesses and 92 authenticated complete objects; all 94
  actual original-flag compiler dependency closures are retained. The 223 current
  required source/header bodies match the qualified freeze, with only the
  previously authenticated canonical-LF coin bridge. The later Collector header
  is outside all 94 closures.
- Primary independently authenticates the 451-file owner seal and 6,425 raw/mode
  source members. Integrated runner generation is byte-identical to the native
  harness that passed. Canonical-LF Python AST and C++ token/literal bridges
  preserve the qualified inputs through formatting.
- The earlier complete physical publication execution remains **60 PASS** across
  both profiles; its source-bound harness is carried into the maintained runner.
  Physical and owner results are separate executions, not a claimed combined
  all-scope run.
- Maintained accounting, writer, route and source-site contracts pass. Scoped C++
  formatting and whitespace are checked before publication.

Primary integration receipts are under
`tmp/coin-owner-fixture-integration-primary-20261007/`; the read-only independent
owner verifier is
`tmp/coin-native-owner-replay-fixture-20261007/verify_handoff.py` (SHA256
`273c265333bdce54ad242feac1be1a5863153194ed0d1f2d52893dd87818efde`).
The immutable owner seal is
`tmp/coin-native-owner-replay-fixture-20261007/FINAL-EVIDENCE-PINS.json` (SHA256
`dd8954627f5b38e2e88dd96b71b8030ed3efaf3b85bf8a3267b261191b7ad8ce`).
Prior failed link/input attempts and the 66/70 floor-slot failure are preserved;
the correction changes neither the original case oracles nor safety gates.

## Remaining acceptance

These are native components with controlled warm world/coordinator seams, not
successful real SQL/native publication or guarded ACK proof. The separately
qualified real SQL coin journeys remain documented in
[the post-ACK report](COIN_POSTACK_MAINTAINED_INTEGRATION_2026-10-07.md).
Flatfile parity, other Plan 2 routes, Plans 3-4, activation composition and complete
R1-R8/release qualification remain open. No writer coverage or release gate is
promoted by this fixture repair.

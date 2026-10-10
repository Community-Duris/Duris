# Snapshot decoder invalid status results - 2026-10-10

Both original and bounded player snapshot decoders could return `ok` after a
status integer/string field exceeded its existing enum range. The row callback
returned false without changing the genuine decoder result, so the main decoder
returned success before transferring its candidate. The bounded retained-heap
output also remained untouched. Encoder self-validation inherited the refusal
bug; real loaders' secondary identity checks did not repair the public result.

All four range checks now assign the existing `invalid_value` result after a
successful field read. Truncated field/payload and resource results remain
unchanged. The complete snapshot and bounded heap scalar still transfer only
at the original success tails; valid bytes, caps, wire versions and accounting
admission/activation policies are unchanged.

The existing player-save journal harness now includes integer/string first-invalid
and maximum IDs, incomplete fields, valid/invalid fields without payload, valid
upper bounds, strong output address/content/heap preservation, encoder refusal
and supported-profile initial resource refusal. These are real decoder/harness
cases prepared for the major-plan batch, not executed native acceptance.

Independent RAW and installed source/test reviews pass. Whole source/test
forward/inverse transformations, explicit test CRLF-to-LF alias, changed-line
C++ formatting and Python AST syntax authenticate. Evidence is under
tmp/snapshot-result-integrated-20261010 and the immutable worker packet
tmp/snapshot-decoder-result-owner-20261010 (manifest b4bcadda).

The registry refreshes the codec pin and adds the existing qualification test
pin (456 total). All 931 writer policies and original site coverage survive;
protected Plan 5 edits stay excluded. No native compiler, decoder, gameplay,
persistence or recovery execution ran for this successor. Pending command:
`python3 tests/async/test_player_save_journal.py` on the supported candidate,
followed by the applicable major-plan build and recovery checks.

Full producer/global-budget/backend/native/R1-R8 qualification remains open.
Accounting stays inactive, admission CLOSED, coverage incomplete and release
BLOCKED; the goal remains active. This repairs the source result contract and
adds characterization; it does not complete a major plan or runtime gate.

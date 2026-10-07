# Plan 5: fixture item-event identity and order - 2026-10-07

The independent fixture auditor now refuses malformed item events through
`AuditError`: non-object rows, missing or noninteger/zero/out-of-range UIDs,
noninteger or out-of-order event indices, and events naming another operation.
Valid repeated events for the same UID and exact whole-operation replay remain
accepted. This fixes fixture grammar and event binding; it does not establish
complete native holdings/provenance or release qualification.

## Source and ownership

- Branch: local and remote `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `ab41214a15acfa542555402935c3fa6c43101661`. Result: the commit containing this report; exact result,
  remote equality and preserved ancestry are recorded in
  `D:/CodexEvidence/accounting-plan5/bin/invariant-item-identity-delivery-01-20261007/delivery.json`.
- Tested code tree: `4d181272e2d2711fe719870ed26263e6fb789438`; source archive SHA256
  `1d5a4502b72a1ff661d855ce04cf8ecdf3abb27e164d4d68b4d15c8117ee9589`. All 6417 regular bodies/modes and
  four original links are pinned and checked again at terminal completion.
- Owned code: `scripts/audit_accounting_invariants.py` and
  `tests/async/test_audit_accounting_invariants.py`. Publication adds this report
  and updates `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`.
- Native tree remains `4abb609524a1f1682ea4c190f82d75003c4d679b`; migrations 0-62 remain
  `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`. No native, migration, shared contract, coordinator,
  shared runner, writer registry/matrix, activation or producer code is edited.
- Refreshed and separately tested primary source: `74b25fc9bc8d0f3bb4e62555cb05d091a7028d04`,
  archive SHA256 `84e2ed067bec8fc911c61a361470243d56027d3ff27136042ceb665a73a307d7`, native tree
  `d149afce4392056ee92afdde2c42dfd22880a5c3`, migrations 0-64 `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.
  Its 6437 regular bodies/modes/four links are independently
  preserved. No checkout or branch switch is used to run this native archive.

The preceding consumption slice is now published as `ab41214a15acfa542555402935c3fa6c43101661` on this same
remote branch. Its delivery SHA256 is
`a53a52a761e11df5d327b81d6f28c8997b25eaebead4eddcf8a2decf5d809086`. All seven earlier alternate
tips and follow-ups remain preserved ancestors.

## Established defect and complete owned fix

The unchanged base auditor accepts eleven deliberately corrupted item-event
fixtures: zero, negative, overflowing, boolean, floating-point and textual UIDs;
boolean/floating/out-of-order event indices; another operation identity; and a
duplicate event index. A non-object event raises an uncaught `AttributeError`.
The original twelve input bodies, outcomes and source hashes are retained under
`invariant-item-identity-red-01-20261007/`. Source and supplied fixtures remain
unchanged. The corrected reader refuses all twelve through `AuditError`.

For each event, the UID must be an exact integer in 1 through uint64 max, the
event index must be an exact integer matching its zero-based position, and the
operation identity must exactly equal the validated containing operation.
Malformed input produces the existing CLI `AUDIT FAILED` result with exit 1
and no traceback. No data is corrected and no mutation provider is imported.

The native item validator permits successive events for the same UID when their
indices and state/revision progression are valid. The fix therefore removes the
unused transfer set and validates event identity/order. It preserves repeated
UIDs within a root, the existing across-root auction history and exact receipt
replay. Valid scalar boundary cases include 1, 2**53+1 and uint64 max without
floating-point conversion. This field-format acceptance grants no native writer,
origin, lifecycle, publication or recovery authority.

Seven focused methods extend the existing mandatory regression suite. They
exercise UID representations/bounds, event ordering, operation binding,
non-object rows, valid repeated-UID events, exact replay and real CLI refusals
with unchanged input bytes. Every prior valid golden fixture and statistic stays
valid. No new central registration or coverage policy is allocated here.

## Exact checks and evidence

Private native checks use network-none containers with CPU 2/memory 4 GiB and
image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Actual host and container termination and completed artifact copy precede sealing.

| Owned-source command | Exit | Duration |
| --- | --- | --- |
| `python3 -u -B tests/async/test_audit_accounting_invariants.py -v` | 0 | 0.303491s |
| `python3 -u -B /evidence/native-original-invocation.py` | 0 | 32.777103s |
| `python3 -u -B scripts/audit_accounting_invariants.py -v` | 0 | 0.033646s |
| `python3 -u -B scripts/validate_economy_accounting.py` | 0 | 3.604803s |
| `python3 -u -B scripts/validate_economy_accounting.py --release` | 1 | 0.051085s |
| `python3 -u -B -m unittest discover -s tests/async -p test_economy_writer_coverage_contract.py -v` | 0 | 5.689615s |

The invariant suite has 23 methods and the writer suite 55, both with zero skips.
Normal validation passes; `--release` intentionally refuses missing writer
execution evidence. Python AST and staged whitespace checks pass.

`python3 -u -B /evidence/native-original-invocation.py` invokes the original
`test_economic_accounting_types.py` main and both original
`test_economic_accounting_item_reference.py` methods. It preserves the original
strict C++20/ASAN/UBSAN flags and retains temporary outputs for authentication.
The historical-source invocation supplies the one missing existing SHOP manifest
provider only in memory. Its two binary hashes, 66
compiler inputs, raw `-MM` dependency outputs, generated golden include, original
and actual compiler commands and runtime checks are retained. The original
unmodified recipe failure is retained as `invariant-item-identity-green-01-20261007/`.
It is never promoted to an original-runner pass.

Seven compiler dependencies differ on the refreshed primary, so the historical
native pass is not used to qualify that source. Separate original and partial
SHOP-only primary recipes both fail to link. The complete four-provider proposal
then passes on the exact unmodified primary archive in
37.254747s. Its original accounting-types
program verifies golden coin/item effects, 5,000 transfers, a 3,000-item forest,
limits and rejection checks; both item-reference methods pass with zero skips.
All 75 current compiler dependency
inputs and both current binary hashes are authenticated independently under
`invariant-item-identity-primary-native-proposal-02-20261007/retained-native-tests/`.

No maintained C/C++ source changes occur in this slice, so no new server Make
build is required by AGENTS.md. This Python change does not alter the preceding
maintained build's native inputs. It does not claim a newer full-server build.
The native programs are pure item/accounting validation and NoMySQL refusal
checks. SQL schema declarations are inspected by the original reference contract;
no disposable MySQL/MariaDB runtime, migration or gameplay journey is selected.
Both-engine current-candidate qualification remains required, not waived.

Seal: `D:/CodexEvidence/accounting-plan5/bin/invariant-item-identity-seal-01-20261007/evidence.json`.
SHA256 `c1e492a14745e61f5eb812e8ff3e8867e6f557a2047aee5552f87c86d3dbc1ca`. It binds 147 artifacts / 1602955126 bytes,
including all original failures, successful controlled proposals, immutable source
archives, compiler closures, binaries, controls, logs and the primary checkpoint.
Delivery verifies every nonpublication body/mode/link against the tested owned
archive. Generated inputs, logs and binaries remain outside committed source.

## Narrow shared runner handoff

On primary `74b25fc9bc8d0f3bb4e62555cb05d091a7028d04`, the original
`tests/async/test_economic_accounting_types.py` compiler list omits existing
providers now referenced by `item_transfer_command.c`. Primary should add these
to that original compiler list and rerun the maintained recipe:

| Existing provider | Required functions |
| --- | --- |
| `src/economy/shop_trade_recovery_manifest.c` | `shop_trade_recovery_forest_shape_valid/freeze/encode/decode` |
| `src/item/lockpick_retirement_continuation.c` | `lockpick_retirement_payload_valid` |
| `src/economy/native_quest_cost.c` | `native_quest_cost_projection_encode/decode` |
| `src/economy/native_quest_coin_give.c` | `native_quest_coin_give_project/encode/decode` |

The exact proposed compiler command adds only those four source files before
the existing crypto link argument. All other flags, source arguments and program
bodies remain original. The consumer is the existing main() compiler recipe;
accounting fields, schemas, wire formats, invariants, inactive and spell behavior
change by zero. The historical native source needs only the SHOP provider; the
three newer files do not exist there. No speculative compatibility fallback or
shared file edit is made. The original primary recipe remains unfixed until
its owner applies and qualifies that source-list change.

## Remaining gates and curator packet

Full Plan 5/R7/R8/release remain incomplete: current combined native/schema and
both-engine qualification, native holdings and complete UID/origin/history
comparison, real admitted writer/player/publication/ACK/cold-recovery journeys,
release-host backup/restore/retention and complete route execution evidence.
The successful fixtures and controlled native unit proposals are scoped proof.
They do not certify the current combined candidate or replace genuine journeys.

The report, follow-up, seal and remote delivery form the curator packet for the
primary-maintained local notebook. Notebook application, acknowledgement and
cross-chat notification are unclaimed and nonblocking. Required plans/checkpoint
are pinned; AI_CONTEXT.md remains unavailable after the recorded searches.
Accounting inactivity, wallet-root exclusions and the declined inactive spell
change remain preserved. No production data, deployment, activation, merge or
audit autocorrection occurs.

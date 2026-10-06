# Prep verification and integration commands — refreshed 2026-10-06

Current accounting source: `2c4e17f363ecff0f0d7eb3451ddb229abdd63d9a`.
Original base: `17c033d69316b21da8598791fc95cae79baa8dc2`.
PR #678 research: `55905eac1906cf59405764407f9d22497cccfff3`.
Upstream was merged as `a28763fcb66a88a286136df1503bd8cc0ed2f72e`, preserving
all original prep commits. No prep-authored change is outside the owned paths.

The delta installs native birth/quest production owners and their test companions.
Production areas, catalog script, selected quest dispatch assignments, interpreter,
world-quest callbacks and ADD_MONEY are unchanged from the original base. The
current source pin supplies the actual owners; historical private-source handoffs
are interpreted together with NATIVE_BIRTH_RECOVERY_MAJOR_QUALIFICATION_2026-10-06.md.
Its reported production builds and codec checks retain their own candidate/scope.

Tools: WSL Ubuntu22.04, Python3.10.12, g++11.4.0, C++20
`-O0 -Wall -Wextra -Werror`. Extracted components compile in temporary directories
under `bin/tests/` and clean up. No server boot, DB, migration, activation or native
cold journey ran. The major-batch gameplay cadence is preserved.

## Rerun results

Run from repository root on Linux. On this Windows host, prefix commands with
`wsl -d Ubuntu-22.04 --cd '<absolute-worktree>' --` and use the same Python arguments.

| Exact command after `python3 tests/async/quest_accounting_prep/` | Result | Scope |
| --- | --- | --- |
| `test_production_terms.py` | PASS, exit0, all seven | Production catalog/terms/reset declarations; five static mini fixtures and two full-world run-roots |
| `test_native_selectors.py` | PASS, exit0, QP01/02/05/06 | Current native availability/selection slice; exact order/count/unique UID/spare/shortage; constructed NPC holdings |
| `test_recipient_retirement.py --acceptance` | PASS, exit0, QP03 | Actual observe_give body; missing/replacement generation and wrong root refuse; lookup/census isolated |
| `test_bartender_settlement.py` | PASS, exit0, QP04/07 observations | Actual unchanged callback and ADD_MONEY; injected hypothetical committed debit |
| `test_native_selectors.py --case QP02 --acceptance` | RED, exit1, component30 | Current paid gloves branch refuses before availability, native pulse blocks; supported backpack remains unreachable |
| `test_bartender_settlement.py --case QP04 --acceptance` | RED, exit1, component30 | Active ADD_MONEY refuses refund credit after refund prose |
| `test_bartender_settlement.py --case QP07 --acceptance` | RED, exit1, component31 | Stale map callback affects task B; observation mode also demonstrates stale abandon history/reset |
| `case_data.py --output docs/persistence/economy_accounting/quest-prep/SOURCE_FACTS.json` | PASS | Refreshed pins, raw hashes and per-case current runtime owners |

All six owned Python files parse; source snapshot raw hashes match the worktree;
`git diff --check` passes. All seven native journeys remain pending.

## Four diagnostic dispositions

- **QP02: obsolete helper replaced, defect still reproduced in current owner.**
  SQL CMD_GIVE now routes to submit_native_quest_give. The component extracts original
  source text for prepare_original's preliminary availability and destructive selection,
  ending before literal custody/payload preparation. With three hides, runtime
  first branch gloves returns refused; direct backpack returns ready with three
  exact roots. Current pulse advances only not_matched and blocks refused.
  The test does not simulate admission, wallet, freeze-program persistence or pulse
  execution. The driver relation is source-verified. Shared recipe/coin authority
  policy remains a primary decision; guards must not be relaxed by prep fixtures.
- **QP03: obsolete lookup seam, replacement predicate resolved at component scope.**
  Old quest_mobile_for matches prototype/room, but current native GIVE owner uses
  original runtime generations through observe_give and canonical birth references
  through quest_mobile_native_reference_copy. The updated full observe_give body
  refuses a replacement with identical prototype/room and independently owned
  same-kind lance. Failure leaves output pointers unchanged. Binding canonicality
  and entry connectivity are source checks. Old finish_quest_reward D cleanup is
  no longer tested as SQL-native retirement. Authentic residual stock/cash,
  disappearance, cold adoption and paired ACK/retirement remain unqualified.
- **QP04: unchanged callback seam, refund predicate still RED.** Actual active
  regular currency preparation excludes generic service spend/refund reasons.
  The test injects a hypothetical successful debit; it proves no actual active
  player charge. Restitution/held recovery still needs the real service owner.
- **QP07: unchanged callback seam, task-attempt predicate still RED.** Payment
  context still holds action/fee/giver, with no original start/target/attempt.
  Replacement B passes boolean readiness and receives map or abandon effects.
  Genuine fault/replay/cold evidence awaits attempt-bound service authority.

## Fixture preparation

```bash
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP06 --output /tmp/quest-prep-QP06
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP04 --output /tmp/quest-prep-QP04
```

Use any QP01–QP07 with a new/empty output directory. Static fixtures preserve
selected production prototype/Q bytes, full QP02 family, original recipient
G/E stock and one spare supplied root per kind. They relocate the recipient to
mini22800 and use synthetic O supply. Native GIVE accepts each offered item into
original NPC stock before consumption selection. Synthetic supply is no proof
of birth origin, source admission, caps or acquired player custody. QP05 cap-one
huge-skin availability remains explicit. Held/capture/submission refusal lies
outside the new selection slice and must be checked in native integration.

Dynamic fixtures reuse run_world_quest_dual_backend.setup_run_root: full production
areas links, isolated lib/journals/local TLS, actual Quietus commands and missing
hook metadata. No static QST surrogate, task, player, epoch or receipt is seeded.
The maintained driver defaults to level56 at Woodseer16633/giver16553; Quietus
requires genuine room1734/giver1709 arrival and quoted fees. Level11 fee examples
are controlled inputs. Generated output is not committed.

## Remaining primary-owned hooks

1. `run_quest_reward_ack_crash.py` fixture/VNUM/alias selection: use QP06 Kord
   ear29262/scalp29263/toe29264, output29237 and C3000. Existing driver fixes
   synthetic acorn/branch/feather, 22802–5 and C1000. Retain its isolated SQL,
   GDB offering/XP-ACK faults and two boots; add genuine later reward custody change
   and acknowledged historical parent/child pair evidence.
2. Native reset/birth/source/cash/forest and recipient D integration: installed
   production code supplies capabilities but these fixtures have not executed
   actual producer, SQL, publication, body-loss restoration or retirement. Observe
   original incarnation and independent replacement stock at each fault cut.
3. Native recipe reachability/mixed-fee policy and bartender exact debit/refund/
   task-attempt ownership. Preserve active refusals until actual coupled authority.
4. Shared writer/source registries, manifests and finish handoff belong to primary;
   Plan5 audit/restore remains independently owned. Record consumed prep commits
   and qualify the actual integrated binary/schema at the agreed batch boundary.

Source/component PASS is not native journey, full Plan, activation or release
qualification. All original R1–R8 and fresh-world SQL-first release requirements
remain. No birth binding, accounting root or success authority is fabricated.

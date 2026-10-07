# R7 superior all-stat plan reservation — 2026-10-07

Proposed substantial owned planning boundary; **no production or maintained test
edits before coordinator boundary review**. Owned source77b442440e8b7599a62c9a1e5ea2932d470813d0;
primary91cd229222fa7c8e8b57bdd8f1a22b856af770be preserves Collector daa5.
R6 final review PASS is acknowledged. Actual continuing Goal remains ACTIVE.

The post-R6 statement that a new capture grant necessarily blocks this existing
producer extraction was too broad. User authorization covers a separately
reviewed compatible domain boundary. Existing native observations can supply
facts synchronously at their original call points; this grants no new source,
publication or recovery authority. This reservation corrects that feasibility
assessment rather than expanding to a new native route.

## Outcome, files and local observation protocol

Move the complete ordered superior planning control into new
`src/economy/enhancement_superior_plan.h`, connected through the existing
`build_superior_enhancement_plan` wrapper for both actual do_enhance execution
and aggregate-preview callers. Move its existing two plan structs, native fixed
capacities and material aggregation law once into that header. Keep field types,
array dimensions and layout identical using core/config.h's MAX_OBJ_AFFECT.
Reuse R4 material quote; do not duplicate or rewrite aggregation or numeric rules.

The header's domain-specific synchronous templated planner accepts a local
observation provider and the existing plan. It owns reset, ordered iteration,
skip/continue decisions, quote admission, sequential low/high aggregation, slot
insertion and maximum remaining count. It stores no provider/game pointers and
introduces no dynamic allocation, std::function, backend framework or lifecycle.

In `src/item/enhance.c`, the existing wrapper supplies a local provider whose
methods retain the original native expressions and call points:

1. `eligible(slot)` evaluates the original short-circuited location/modifier/
   is_superior_stat_apply checks. It does not snapshot all slots.
2. `base(slot)` reads the current APPLY location and invokes the unchanged
   enhance_base_modifier. Its prototype read/first matching APPLY/cleanup stay
   there. `cap(base)` calls the same enhance_stat_cap only after that returns.
3. `modifier(slot)` supplies the fresh affected modifier only in the original
   base<=0-or-modifier>=cap short circuit. No initial cached modifier is reused.
4. `target(slot)` invokes unchanged find_stat_enhance_target with fresh current
   location/modifier, then original read_object/get_matstart/extract_obj. Only
   after cleanup does it supply an available flag, low VNUM, target item value
   and current quantity multiplier as owned numeric facts. Null target/unreadable
   object skips before material/config reads or cleanup. No target pointer enters
   the header. The original low+4 high-VNUM calculation and R4 quote remain in
   sequence before aggregation. No range/eligibility policy is added.
5. After both aggregations succeed, the header appends the original slot index.
   `remaining(slot,cap)` then invokes unchanged superior_stat_remaining_steps
   with freshly read location/modifier. The header takes the original maximum.

This split preserves per-probe re-observation. Low aggregation may update the
plan before high aggregation fails; partial output on failure remains intentional.
No later slot or remaining probe occurs after quote/aggregation failure. Initial
plan bytes are cleared, including on an empty/no-upgrade result. Preserve original
non-null preconditions; no new success-only output or whole-command atomicity
promise replaces the current plan behavior.

Extend existing `tests/async/test_superior_enhancement_material_bounds.py` with
the actual header/types, original/extracted native wrappers and identical ordering/
partial-plan controls. Relocate only the moved material/planner source assertion
in existing `tests/async/test_enhance_all_stat_contract.py` to the real header and
wrapper. All payment/effect/preview/config assertions remain. These are the exact
four implementation files; no new maintained runner, shared fixture or test
manifest is introduced.

## Current preimages and executed original feasibility proof

Complete owned builder SHA256
`84c17fc29593ee089d77d3ab580f5359ba1fc06160b38ccb9ada5dd4747d7571`;
aggregation `f23e48aa7f6ae030a9251ef2c1c6f6cd354eb65540afd00921ef94e7a02895f2`;
existing material regression
`1fe33d11733323a9cb8069ef982ca5b9ed5abbaaa11567a226f519d916385f1e`;
all-stat contract
`ad2c23e04d035d04a98f9d2609a07423092bba44bb5802e49533a87070906c9b`.
Native base/target/entry-modifier/remaining helpers are separately body-pinned in
ignored bin/tests/domain-r7-feasibility-20261007/preimages.json and remain untouched.
The recent primary milestones do not change their native observation bodies.

Private original-ordering.cpp compiles the COMPLETE unchanged builder and
aggregation with all existing material controls and ten added trace scenarios.
Executed PASS using the original C++20 -Wall/-Wextra/-Werror, -O1, ASan/UBSan/
float-cast-overflow, no-PIE and30-second execution budget, in the existing owned
Docker container. Original source, generated harness, trace assertions, log and
hash index remain ignored evidence. Production/tests are unchanged. This initial
feasibility proof uses instrumented native helpers; it is not a native journey
or full prototype/catalog qualification.

Executed scenarios establish: all-slot ordering/MAX; original skip short circuits;
cap still queried after base0; second-slot quote refusal with no later probes;
high duplicate overflow retaining successful low insertion; capacity refusal
retaining low insertion; target-cleanup changes to value/config/current stat
observed at their original subsequent points; base-probe modifier changes; absent
target/unreadable target skips; and next-slot mutation during remaining queries
read afresh. No assertion, compiler flag or timeout is weakened.

For final qualification, run identical extended expectations against original
and extracted complete builders. Extend this same runner with genuine extracted
unchanged base/target/remaining helper bodies where necessary to establish unreadable
prototype handling, first matching APPLY, cap config changed during prototype
cleanup, target value/config changed during cleanup, lowest-VNUM/first-linked ties,
zero effective wear, and contiguous remaining probes stopping at the first gap.
Compare complete plan outputs, partial failure bytes and native call traces.
No synthetic source/receipt/publication success is supplied by these components.

## Non-goals, import and proportionate proof

Native source identity, prototype/catalog lifetime, read/cleanup helpers, RNG,
payment, inventory/pouch consumption, output effects/marker, active refusal,
publication, receipt/ACK, persistence and recovery stay unchanged. Preview remains
aggregate-only. Other domain owners, shared source/tests/manifests, migrations,
quest-prep paths and declined spell behavior remain protected.

The production and full test package explicitly reuse R4's quote header/test
predecessor. Check application on the exact current primary plus R4 alone;
identify incidental context separately. Do not claim bare-primary independence
or silently require R3/R5/R6. Native cap wrapper works with either its original
body or reviewed R5; no new scalar rule is part of R7.

Retain original material/payment/stat/config/pool and actual active-refusal
controls; preserve the known original module reset-count failure. Run both original
maintained SQL/flat builds, repository formatting and current composed full-module
type checks. Pin source, controls, commands/terminal logs and actual ELFs separately
from current754-provider source scope. Genuine supported native journeys and full
current overlays remain separately declared rather than inferred from components.

Ablation removes eager slot/catalog/target snapshots, a generic provider framework,
new public hooks, all-or-nothing aggregation, source validation/recovery redesign
and extra test files. The retained local protocol is necessary to extract the
full planning operation without moving observations past earlier failures. Reuse
the existing plan, numeric rules, native helper bodies and focused runner. This
reservation and any delivered R7 do not close the broader integrated Goal.

## Original native-helper feasibility supplement

After reservation publication cf689eb7a, the same private harness was extended
with the COMPLETE unchanged extracted base/entry-modifier/target/cap/remaining
helper bodies (renamed only for coexistence with traced wrappers). Seven further
original cases PASS under identical flags and30-second limit: actual prototype
first matching APPLY, unreadable prototype with cap0 and no cleanup, cap config
changed during prototype cleanup, current modifier changed during that cleanup,
target-cleanup value/config/stat changes followed by contiguous remaining probes
stopping at the first gap, lowest-VNUM/first-linked ties with zero effective wear,
and nonzero incompatible wear refusal. Full plan outputs and exact native traces
are asserted. Original controls and ten prior ordering cases still pass.

The native supplement is separately indexed as native-proof-pins.json; the
original feasibility hash index is preserved. This remains controlled component
proof with actual helper bodies and instrumented read/cleanup endpoints, not
native lifetime, physical source/publication or gameplay qualification. No
production/test edits are made before boundary review.

## Implemented qualification checkpoint

Boundary approved at accountingc04a0faead4775863d408e1224e1668f869198e1.
Implementation bf8ffda1d6bae94866c6cb2834d0561d7d914ed9 and
[R7 terminal handoff](R7_SUPERIOR_PLAN_HANDOFF.md) deliver the exact four-file
operation. Original/extracted21-scenario controls, adjacent checks, both owned
maintained740-object builds and current/R4-only-prefix component/module checks
PASS. Initial routing failure and unchanged module reset-count failure remain
retained. Final review pending; the proposal language above is reservation history.

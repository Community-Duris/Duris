# Original shop codec fixture closure investigation — 2026-10-07

Disposition: **private canonical-provider experiment PASS; maintained repair
reserved for coordinator review, not yet authorized or implemented.** This is
new independently actionable qualification work assigned by the coordinator,
separate from blocked native quest journeys. Actual continuing Goal still reports
BLOCKED (createdAt1791384873, updatedAt1791387688); no replacement Goal or full
completion claim. Primary and Plan5 retain authorities and native acceptance.

## Source and original failure authentication

Immutable primary export: e6e058515f5433a1a3028f80d5d7d672d48b2471.
Git archive SHA256: 13e90d1ceb9edbfd33e7f2fe3389ce2ae4fa8abcbb54f32365a5c13f90c1a2ab.
Later publication98fd2abed0dd7332cb55781c85882aaacf0a5d67 changes only the finish
plan, continuing charter and R10 review; no production/test input change. Keep
actual e6 experiment pins, rather than relabeling it or rerunning unchanged code.
Prep documentation basecbdd5f8837db868796587d41f2153748fd71bdc1.

Original tests/async/test_shop_trade_command.py Git blob
51449801c538c412be86671bc4789dbdb81b6602, SHA256
faf2d31b66cd77ab7678d6b4e69ddd96440b9c90f7d1468e2c8d2fc56cce9ca1.
Generated complete HARNESS SHA256
f054d84ec7e76103001404bab2dc723cac42fdf34cdccd30dd0bc12b5c1ddf0a:
46 original assert statements. Original script and source files remain unedited.
All ten original compiler-input hashes and generated harness bytes exactly match
architecture's supplied adjacent-baseline-comparison-RESULT.json (copied privately,
SHA256 addd3cdefae9f71b5ce353ba8cd6834da872bfaa7e757e8899672c38b025b32b).
The earlier two raw branch-input differences reflect ancestry; the actual
bare-primary comparison agrees with this current immutable export. R10's shop.c,
quote header and purchase runner are not inputs of this command-codec recipe.

Execute the **complete original Python runner**, with its exact generated harness
and ten sources. Original g++ link exits1 in7.028s; overall FAIL7.090s. All six
reported canonical function names reproduce: lockpick_retirement_payload_valid,
native_quest_cost_projection_encode/decode and
native_quest_coin_give_project/encode/decode. Original stderr, command, controls,
harness and result remain retained. No original runtime ran before link success.

## Actual provider contracts and smallest closure

| Existing canonical source | SHA256 | Dependency/contract |
|---|---|---|
| src/item/lockpick_retirement_continuation.c | fcf706b98339b2a5266e3ea1897c93d399820c621a85c0aef99dcaf14007e0a3 | Defines lockpick_retirement_payload_valid (line71). Validates original destruction/single-root identity, actor, continuation, revisions and decoded literal ITEM_PICK/HOLD shape. Uses player_item_snapshot_list_decode; player_snapshot_codec.c is already in the original recipe. |
| src/economy/native_quest_cost.c | 036978c11d968b5dcda06e7bbc06ee7715441b4bc9c433a4b26879f16a8e2ca5 | Defines canonical cost projection encode/decode (lines173/220), retaining exact native cash/value bounds and original projection validation. |
| src/economy/native_quest_coin_give.c | 88543581facf2dca16e646a7be068539106e956142f6ec3eb473e32b5bab26f3 | Defines actual project/encode/decode (lines30/72/116), retaining denomination/quantity/revision/overflow controls and original player spend plus normalized recipient credit. |

item_transfer_command.c directly references those definitions: validator line1061,
native cost at1684/1762/1838/1897/2645, native money at1966/1996/2061.
No new provider API, authority, decoder, accepting/no-op stub or field omission.
The three actual sources close the complete original recipe: no further provider
was needed. nm confirms all six real definitions as text symbols in the final ELF.

## Executed private experiment and controls

Ignored host evidence directory:
C:/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max/bin/tests/shop-codec-closure-e6e-20261007.
Networknone container quest-prep-shop-codec-e6e, 2CPU/3GiB; immutable source archive
unpacked as /candidate, evidence bind /evidence. Original pinned QA image
sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b,
g++ Ubuntu13.3.0-6ubuntu2~24.04.1. No DB, player or server starts/operations.

Actual command:

```text
docker exec quest-prep-shop-codec-e6e python3 -B /evidence/investigate.py
```

The ignored observer executes the unmodified runner with runpy twice. First
invocation records the original subprocess command without modification. Second
invocation inserts ONLY the three source paths above immediately before -lcrypto;
removing that insertion yields the exact original command in the same invocation.
Source/harness material is copied before original temporary-directory cleanup.
Original check=True/capture_output=True/text=True compile controls and original
check=True runtime control are unchanged. The original runner has no explicit
subprocess timeout; none is substituted. Flags stay exactly
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -Isrc`, link `-lcrypto`.
No NDEBUG, optimization, linker GC, sanitizer or macro alteration. Original
semantic fixture and all assertions are retained; no reduced smoke harness.

Closure compile/link exits0 in7.772s. The complete original executable exits0
in0.003s; original runner emits `[PASS] bounded recoverable shop-trade command codec`.
Overall **PASS7.799s**. Both actual generated harness hashes are identical.
All2646 C/C++/header/Python source inventory entries under src and tests/async
are unchanged before/after the experiment. Final ELF SHA256
e45bc8def3323a2ba454195568cfeae31d5405a4eabe4df01eed3d60b7ab7ebf.
This qualifies original bounded codec/fence/version/result assertions only;
it is not actual native shop, quest, backend, birth, custody, publication or ACK proof.

Private reproducibility and evidence hashes:

| Retained file | SHA256 |
|---|---|
| investigate.py | 8e62fc4b182bb2b79bfe0d55b9b8615704fbb3176e8d5aee5f910fb760946414 |
| RESULT.json (commands, controls, both results, full source inventory) | 938d5b56c22aa5f8d06b858ba18664ef7c6ea0ccb215a7a7e04a05c68715ff56 |
| summary.json | 930e44a63657cde700cfa0efe0f3977066bc421495814f6f175048d10889c15f |
| original/stderr.txt | 9735c83088b8f5da4fd7902b7b570a7c238a73b4af3baf7a4622dd9becbbef3c |
| original/result.json | e6ea460c06b5c1c7f1333ad2087eb47958fb1bfd24af7c589296d4c47b535391 |
| closure/result.json | 47e56ebf6b2ff0c509f026f6817e758ca496bbd82a074d47907b5b12f0e08abe |
| canonical-provider-symbols.txt | 17920c79a2a652e3d02640e0b2cbdc1de99985a8e43cc26a4a8a4394dd9e58ca |

No private artifact, raw log, source archive or ELF is committed. Retain original
FAIL and separate private closure PASS; the maintained original test still fails.

## Precise ownership reservation for review

Requested future authored path: **tests/async/test_shop_trade_command.py only**,
its existing g++ input list. Add rel("lockpick_retirement_continuation.c"),
rel("native_quest_cost.c"), rel("native_quest_coin_give.c") after original
critical_command.c and before original -lcrypto. Preserve original ten sources,
HARNESS, fixtures, assertions, flags, link library, controls and temporary cleanup.
No production source/header, shared registry/manifest, authority, schema, R10
shop extraction or Plan5 file is reserved. No new maintained test framework.
Current preimage is the exact e6 script/blob/hash above (unchanged in98fd).
After a separately agreed ownership boundary, execute the complete maintained
runner at the reviewed current primary pin, retain original failure and publish
the one-file coherent repair with its actual result. No such shared edit is made
by this investigation. Coordinator reviews; primary adopts/qualifies independently.

Native lifecycle/reset-born source/custody, command/publication/ACK boundaries,
supported delayed settlement and held-charge/refund/paired-retirement blockers
remain exact unfinished requirements in CONTINUING_RECONCILIATION.md. This useful
fixture qualification does not replace the full continuing Plans1–5/R1–R8 goal.

# Paid repair/smith source-contract handoff — 2026-10-08

The selected two-file delivery replaces the existing smith test's false
"atomic compound transaction" implication with explicit **source-contract-only**
scope. The complete real provider text passes 21 labeled predicates, with the
original writer checks preserved. This executes Python text checks, not C++ or
native service operations. Every updated CLI result reports
`source_contract_only=true` and `native_compound_qualification=false`.

Legacy hazards are **comparison facts, not desired permanent behavior or
architectural requirements**. An intentional primary fix must update the relevant
contract explicitly and review the original comparison and new business behavior.
These checks must not force preservation of a bug, justify an authority bypass
or stand in for native atomicity evidence.

## Frozen inputs and exact ownership

Public `experimental-accounting` is frozen at
`07c0e0398e0123ad9232e162cd1d1df08fd38a6d`, fetched after remote-head verification.
Its source/test trees remain `833d3085815b396861ad18a77635412212381e4b` and
`790f367adf805a69d53aac6460938f5c921f9136`; the reported private `d9eaa45b`
NMB4/storage work is source-report context only, with no new paid-service API or
native qualification adopted here. No private primary source is executed.

The task's fresh archive contains `src`, `tests/async` and the original writers
file: 2,817 original files, 45,731,840 archive bytes, SHA256
`0dcb04afd7b9ac1739869c3f7691a164505e0e76f408cf2e3cf1b811c951d179`.
The complete public export, original test/preimage and all failed attempts remain
unchanged under `D:\Dev\Temp\paid-repair-smith-source-contract-20261008`.
The new test is run outside that export with `--source-root`, so qualification
does not overwrite its original test or provider files.

Only these maintained paths change:

- [`tests/async/test_smith_tradeskill_contract.py`](../../../../tests/async/test_smith_tradeskill_contract.py)
- This new handoff.

Parent is the accepted source/design boundary commit
`9b90753c442818ae9d0c3082c1b4134623987699`. The
[paid repair/smith boundary](PAID_REPAIR_SMITH_AUTHORITY_BOUNDARY_2026-10-08.md#smallest-available-nonduplicate-acceptance-proposal)
supplies the reviewed proposal. Its original observations, implementation order,
native fault cuts and missing authority/fixture dependencies remain in that
document; they are not duplicated as a new runtime design here. R1/R2/R3–R14
math/policy and all closed Collector/Hand evidence remain closed and preserved.

## Complete function parsing and errors

The test reads three complete real files: `src/economy/tradeskill.c`,
`src/economy/shop.c` and `src/item/item_movement_transaction.c`. A small local
lexer discards whitespace/comments, retains quoted literals as single tokens,
and keeps compound operators together. Balanced argument/body delimiters locate
one complete definition for each selected function; declarations, calls and
function-like text in comments/strings do not substitute for it. There is no
4,500-character body window or copied provider implementation.

The nine selected definitions are smith, grant_tradeskill_item,
refuse_unported_shop_mutation, shop_keeper, shopping_repair, accept_gem_for_debt,
transact, item_creation_grant_submit_to_player and queue_creation_grant.
Predicates use token sequences/order and scoped failure blocks. Missing or
ambiguous fragments, absent/duplicate definitions, unclosed bodies and changed
order produce explicit errors naming the real path/function and predicate.
This is not a complete C++ parser or control-flow analyzer. Raw string syntax
is absent from the selected public source and explicitly unsupported; encountering
it fails for review rather than silently truncating a body. A source-contract
pass does not prove arbitrary inserted code harmless or establish native semantics.

## Preserved and added predicates

| Selected source claim | Predicate labels / exact limit |
| --- | --- |
| Original debit < creation < grant < retirement | `smith.debit-create-grant-retire` preserves SUB_MONEY, actual forge_create, actual grant and extract_obj ordering. Earlier obj_from_char detachment is distinguished from retirement. No accepted submission is renamed committed success. |
| Original ADD_MONEY obligation, strengthened to both actual failure blocks | `smith.refund-calls`, `smith.creation-refusal-refund`, `smith.grant-refusal-refund` require both requests and their refund-before-ore-return-before-return order. This proves source calls, not successful refunds. |
| Service and direct guards | `shop.active-service-refusal`, `shop.service-guard-before-smith`, `repair.direct-guard-before-selection`, `smith.direct-guard-before-selection`, `smith.direct-guard-return`, `transact.direct-active-refusal` preserve actual active refusals and the BUY/SELL-only exception. No gate is opened. |
| Original smith identity, menu and custody facts | `smith.keeper-customer-parameters`, `smith.legacy-menu-bound`, `smith.keeper-detach-before-debit`, `smith.customer-ore-return-calls` check actual parameter roles, the existing comparison using i, keeper inventory scan/detachment, and all six original customer-return calls. These are selected source predicates, not a new menu policy or a whole-function equivalence proof. |
| Repair prepayment mutations and condition ordering | `repair.prepayment-weapon-and-condition-order` checks probe read, attack/slash assignments, probe extraction, positive/sub-100 condition gates and later transact. It does not manufacture an all-fields-unchanged refusal oracle. |
| Both disabled gem routes | `gem.selector-disabled` requires the unconditional return immediately before the scan; `gem.exchange-disabled` requires forced-null merchandise before its branch. Later conditional returns are not mistaken for the early disable. |
| Actual grant/queue shape and completion limits | `grant.smith-no-business-callback`, `grant.refused-output-cleanup`, `grant.queue-without-business-callback`, `grant.accepted-is-not-completed` check the real crafting grant call, refused candidate cleanup, null business callback slots and the queued/started/transient true-return branches. No callback/context/format is added. |
| Original writer classification | The existing unittest still requires the crafting.smith entry, reason crafting_cost and symbol smith. It reads the actual original writers JSON; neither registry nor writer data changes. |

The result is two focused unittests, one checking the 21 source predicates and
one preserving writer classification. No price/material arithmetic or completed
R1 recipe/native continuation acceptance is re-extracted or rerun.

## Qualification and retained failures

| Bounded control | Actual result |
| --- | --- |
| Unmodified original public test | 2/2 PASS; its old atomic wording remains in the preserved preimage/log, not adopted as runtime proof. |
| Final updated test against frozen public export | 2/2 PASS, source-contract-only JSON, native compound false. |
| Final updated test against existing isolated checkout | 2/2 PASS with the same explicit scope. Full owned input identities are retained separately; different public/owned files are not relabeled identical. |
| Deliberate source-predicate mutations | 31/31 reject, covering every one of the 21 predicate labels, including deletions and reordered operations/guards/refunds. Each retained error must identify the expected predicate. |
| Preserved writer assertion mutations | 3/3 reject: absent crafting.smith entry, wrong reason, wrong symbol, using the actual unchanged writer unittest on private JSON copies. |
| Complete-function parser negatives | 3/3 reject: absent smith definition, duplicate definition, truncation inside its body. |
| Harmless selected-function formatting/comment controls | 9/9 PASS successively, one per selected definition, with exact token identity and comments containing misleading braces/function text. Quoted tokens and compound operators remain intact. |
| Repeat original parse after the complete control suite | PASS with original sources and predicate labels unchanged. |
| Logging failure/deadline controls | Exit 7 retains stdout and stderr; a 0.2-second deadline returns 124 and retains both partial streams plus a TIMEOUT marker. |

The 37 rejecting controls are private text/JSON/parser mutations. They are never
compiled, linked, installed, or used as production providers, and make no claim
that their altered C++ strings form legal native programs. Formatting controls
operate on selected function tokens, leaving quoted token contents intact.
Mutation specifications, offsets/replacements, resulting hashes and exact
failure messages are retained in `mutation-results.json`; the private scripts
reproduce them from the authenticated original source.

The first updated public attempt failed because a new predicate incorrectly
required a unique `return NULL;` in the gem helper, which actually has three.
Its runner snapshot, command, stdout, stderr and status remain retained. The
correction matches the actual unconditional return adjacent to the inventory
scan. It does not weaken either disabled route or any preserved original check.
Final lexer/operator and guard-predicate changes were followed by fresh final
public/owned runs and the complete mutation controls.

`run_checks.py` launches each focused original/public/owned test with a 30-second
deadline, writing separate stdout/stderr and exact argv/cwd/status/elapsed-time
metadata. The mutation suite has a 60-second child deadline; task temp paths are
explicitly on D:. Subsequent runs use new attempt names instead of overwriting
failed logs. No compiler, backend, DB, server, journal, native, broad-suite,
sanitizer or gameplay run occurs.

## Reproduction and evidence authentication

From PowerShell in the existing checkout, use the committed runner with the
retained original export:

```powershell
python tests/async/test_smith_tradeskill_contract.py --source-root D:/Dev/Temp/paid-repair-smith-source-contract-20261008/candidate -v
```

For the default checkout, omit `--source-root`. The retained wrapper commands
provide the actual external deadline and separate stream logs; the direct test
itself does not launch subprocesses or write provider inputs.

`input-pins.json` authenticates every original exported file by byte count,
SHA256 and Git blob. Final review compares the complete public path/blob set and
all bodies to the frozen revision/archive, verifies original test/preimage,
checks final test/snapshot identity, mutation coverage/counts, final scope JSON,
all command statuses/deadlines and logging controls, and confirms the exact
two-path diff plus Python syntax and whitespace checks. `owned-input-pins.json`
separately binds the default-checkout inputs to its unchanged parent Git bodies.
`qualification.json`, `final-review.json`, `publication.json`, the final document
copy and `evidence-index.json` retain proof on D:. The index includes failed and
successful attempts and excludes the separately authenticated original export.

| Input | Exact identity |
| --- | --- |
| Public tradeskill.c | Git blob `194a393c174c4067557777a88e36f82dcfc7de12`; SHA256 `d71fed61642677307d95bdffeb63a3615a23563f13796814b2ceab68037d70d7` |
| Public shop.c | Git blob `0e63a8b3a72c705bce5030caf6b828b57c8ccae5`; SHA256 `de8bdbe95cfa626c6ca869495ccc34afc0097af4c964d021ad2d66cb99d34393` |
| Public item_movement_transaction.c | Git blob `9d2ce19fd843eef7ce2bda7c41348471b5a2864b`; SHA256 `b41d72ba71a5a80e10d8538d4f272bc87013bfc00ae49f344459ff1ec7ee02a5` |
| Public writers.json | Git blob `71fab7022992e198764b3732dde4936605ed7db1`; SHA256 `92948dcb700bb56482d61c02e3cbe142bd270a9c96ee49cc65e6720aca917664` |
| Original public/owned test preimage | Git blob `88c958e7acfd082fcf856acd05c254a0c578a6e4`; SHA256 `02c0d822a5b9675a463ae00755f3f86b2efe3f9531d2259ff0f1215ebb370533` |
| Final updated test | Git blob `a1bba68f8e65f9d6e16fc88b3108ee02951415c4`; SHA256 `7b25221c38f090bc2fc56d239559ee66367c82d5f5cd1f845573ed9463082751` |

Independent review should inspect the complete parser/predicates, original
source and mutation failures, then verify the retained inputs, commands and
two-file publication. All PASS claims are source-contract scope only. Original
service participants/world identity, outcome capture, currency/item receipts,
save holds, authenticated publisher/ACK and cold SQL/flat drivers still belong
to the native owners. The original continuing Goal remains **BLOCKED and
unfinished**; this delivery neither resumes it nor qualifies compound services.

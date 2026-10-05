# SHOP frozen epoch field repair — 2026-10-05

The Plan 5 maintained SQL build at frozen primary `cfd9c8ab2405920691b9ec0613e0c6217fc93ae7`
failed because `economic_frozen_intent` has no direct `epoch` member. The same
expression remained in published primary `be0877d0c6a47170a98868fca1d6067ae711a1c8`.
The [original failure handoff](https://github.com/Community-Duris/Duris/blob/fbe37541dcf753aea5eb98dc093e7d95daa8382b/docs/persistence/economy_accounting/PLAN5_COHERENT_0056_BUILD_RECOVERY_AND_FAILURE_HANDOFF_2026-10-05.md)
retains the compiler diagnostic and its input scope on the Plan 5 branch; that
report is not yet imported into this primary checkout.

`shop_trade_preparation_owner::build_accounted_command` now compares
`intent.admission.metadata.epoch.bytes` with `prepared.mapping.epoch.bytes`
after successful `shop_trade_accounting_decode`. The existing frozen-intent
contract stores its original operation metadata there. The comparison still
rejects a mismatched epoch before retaining the command. Wallet/bank equality,
original preparation/checkpoint token, accounted schema, and one-time acceptance
timestamp retention are unchanged. No public field, wire layout or schema changes.

Raw source inverse proves this is the sole production expression change;
changed-line clang-format18 preserves it exactly. Current raw source pin and
writer matrix are refreshed without qualifying a route. Static validator and
generated-matrix results are recorded in the milestone receipt after execution.

No compilation, native tests, SQL/services, gameplay, persistence or recovery
checks ran for this repair. The maintained SQL/flatfile builds and original
accounted preparation checks, including epoch-mismatch refusal, remain in the
original major-plan qualification batch. Central admission stays closed,
inactive behavior and the declined spell path are unchanged. Cold publication,
flat parity, coherent schemas and full R1–R8/release acceptance remain open.

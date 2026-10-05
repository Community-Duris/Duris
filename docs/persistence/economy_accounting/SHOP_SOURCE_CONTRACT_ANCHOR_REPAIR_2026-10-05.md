# SHOP source contract anchors repaired — 2026-10-05

The peer's coherent-0056 batch ran 71 accounting contract methods: 69 passed,
one failed and one errored. Both stale probes remained at primary
`7b9c96aa5373f7bca9abeade72a9107cf2563811`. The historical result is retained in
the [original peer handoff](https://github.com/Community-Duris/Duris/blob/fbe37541dcf753aea5eb98dc093e7d95daa8382b/docs/persistence/economy_accounting/PLAN5_COHERENT_0056_BUILD_RECOVERY_AND_FAILURE_HANDOFF_2026-10-05.md).

The checked-placement probe now follows the actual
`shop_trade_publish_physical_impl` rather than its delegating wrapper. It keeps
the original unique `obj_to_char_checked(object, buying ? ch : keeper)` expression,
the complete expected checked-placement set and exact `shop.buy_produced` ownership.

The SQL probe now locates the three original item event writes inside
`apply_item_events` and the bank revision write inside `apply_native_trade` by
their unique SQL expressions. It no longer depends on obsolete line numbers.
The existing helper rejects missing or ambiguous operations. Complete detected
SQL-write coverage, exact route ownership, schema flags, backend refusal and
unqualified release assertions are preserved. No production source or route
qualification changes in this slice.

Exact text and AST inverses preserve every unrelated assertion and method.
Source extraction uniquely identifies all five operations, and each resolves to
its existing exact registry owner. Python AST and whitespace checks pass. This
is source review, not execution of the repaired unittest methods. All 71 original
methods remain required in the major-plan qualification batch. No native build,
database/service, gameplay, persistence or recovery check is run here. Accounting
stays inactive; full R1–R8 and release acceptance remain open/BLOCKED.

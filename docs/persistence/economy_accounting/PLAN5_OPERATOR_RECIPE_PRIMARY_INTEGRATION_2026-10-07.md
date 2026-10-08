# Restore operator provider recipe: primary integration - 2026-10-07

The original restore operator builder omitted three real providers reached by
the retained journal/quarantine payload decoder. Its original link reported six
undefined symbols. The reviewed peer repair adds `lockpick_retirement_continuation`,
`native_quest_cost` and `native_quest_coin_give` to the existing `SOURCES` list.

Primary fetched the actual code commit `1793deb80275f1fba2c6d9a73cebf278f13d54c6`
and imported only its exact builder body onto canonical `71e421d12`. The builder
preimage is SHA256 `68be6e9b01dc6b5688971c5f29b074b8e16ce642463405e54e09e3a935178f86`;
postimage is `39fb56b06201ec0dbb0ce3c6bfea0eac02cc506c12dd4615ac4b53066d4a1ddf`.
All 59 original source nodes retain their order; the new count is 62. Comparing
the complete Python AST after restoring only the source-list value proves that
all other code, compiler flags, options and behavior are unchanged. Each added
provider resolves uniquely to a real current source file. Whitespace checks pass.

The [canonical independent review](domain-separation/AUDITOR_PROVIDER_BOUNDARY_REVIEW_2026-10-07.md)
records the original failure and exact repaired compile separately. Its external
original builder exits zero, with all six missing symbols defined. This is the
same builder body and unchanged production/migration trees; primary has not
repeated that link or executed runtime cases. No passing custody build is rerun.

Local integration evidence is
`bin/tests/operator-provider-primary-integration-20261007/RESULT.json`.
The remote documentation successor was adopted through a new linear branch;
no merge, rebase or cherry-pick occurred. Unrelated worktree changes are preserved.

This closes the three-provider recipe omission in maintained primary source.
Original dedicated tests, native/runtime, both-engine gameplay/recovery, complete
world census, activation, Plans 2-5 and applicable R1-R8 release gates remain.
Inactive accounting, policies, budgets, stubs and source implementations are unchanged.

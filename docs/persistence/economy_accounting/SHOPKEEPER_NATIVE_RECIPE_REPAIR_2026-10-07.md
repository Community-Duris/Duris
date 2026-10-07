# Shopkeeper native fixture provider repair - 2026-10-07

The repository and ownership test recipes linked `item_transfer_command.c`
without all of its real providers. The repository recipe now includes
`lockpick_retirement_continuation.c`, `native_quest_cost.c` and
`native_quest_coin_give.c`. Ownership includes those three and
`shop_trade_recovery_manifest.c`, which was already present in the repository
recipe. This repairs the concrete provider omissions; it changes no server code.

The native transfer implementation calls the retirement validator, native cost
encode/decode, native money projection and SHOP recovery forest functions.
Their definitions exist in these maintained providers. The independent Plan 5
[shopkeeper report](https://github.com/Community-Duris/Duris/blob/a5dac7db92e4e251145f0eb7f2a030f015d8a246/docs/persistence/economy_accounting/PLAN5_SHOPKEEPER_LITERAL_CUSTODY_2026-10-07.md)
also reports the original link failures and successful isolated three/four-provider
proposals against native tree `833d3085815b396861ad18a77635412212381e4b`.
Those remote executions remain reported external evidence: this checkout did not
reopen their raw packets or execute the repaired native cases.

Primary source validation passes. Removing only the three/four added `rel()`
nodes yields exactly the original complete Python AST in each test. All original
providers, compile/link flags, subprocess options, cases and assertions remain.
The ownership fixture's explicit load stub is unchanged; even its native pass
would establish pure reconciliation rather than persistent-load qualification.
Every recipe provider resolves to one existing maintained source file. Central
inventory validates all 926 tests and normal accounting validation passes.
Protected unrelated work remains byte-identical.

Private primary receipt and preimages:
`bin/tests/shopkeeper-native-recipe-primary-20261007/RESULT.json`.
The bounded source-check command completed with exit 0; no compiler, gameplay,
persistence service or recovery journey ran. Per the requested testing schedule,
run both complete original entry points with the major-plan native candidate:

```sh
python3 -u -B tests/async/test_flatfile_shopkeeper_repository.py
python3 -u -B tests/async/test_flatfile_shopkeeper_ownership.py
```

The retained local WSL math-library link failure and unavailable Docker host are
unchanged. No failed full native link was retried or provider stub introduced.
Source-level recipe completion does not close native acceptance, Plan 4, Plan 5,
writer coverage or release. Accounting and the original safety gates stay inactive.

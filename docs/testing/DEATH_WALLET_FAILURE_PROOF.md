# Death-wallet follow-up validation (#251)

The reported ordering was changed by merged PRs #242 and #300. This follow-up
adds missing failure coverage without replacing their runtime implementation.

`test_death_wallet_retry.py` executes the production death retry function with
controlled persistence callbacks. It now injects a nonzero balance after a
conflicting conversion, another balance publication before retry, a failed
disputed-death save, and a failed ordinary terminal save. It asserts that neither
terminal path sees a nonzero wallet, failed saves retain the character/dispute,
and subsequent retries do not convert the zero wallet again.

`currency_transaction_mysql_harness.cpp` adds a real MariaDB full-wallet
conversion conflict: capture a 700-copper conversion, publish 701 copper at a
new wallet revision, then apply/replay the old command. The command is rejected
as stale, the wallet remains 701, the existing pile remains 300, and no ledger
entry is added. The existing matrix checks write rollback, connection loss
inside the transaction, duplicate replay, reload, and player/corpse/locker piles.
This separately verifies the authority behavior represented by the retry test's
controlled callback; the callback itself does not implement a SQL transaction.

## Local results

- Updated production retry test: pass.
- Death-item custody regression: pass, including refused event admission and
  durable custody after large disputes.
- Disposable MariaDB death-disposition schema/apply/replay/refusal harness: pass.
- Updated disposable MariaDB currency schema/transaction/crash/reload matrix and
  player-load repository matrix: pass.
- Real flatfile normal, reset-coin and boons combat/death/save/reconnect/restart
  journeys: pass, including zero authoritative wallet and stable death evidence.
- Real MariaDB combat/disputed-death/restart journey: pass.
- Both local backend development builds and touched C++ formatting: pass.

The player journeys ran against the unchanged wallet/death runtime on master
`1db71f721` with unrelated focused completion/purge changes. This test-only PR is
based on `767e66e2b`; its intervening account-creation change does not alter the
wallet/death code. SQL harnesses ran in the dedicated loopback MariaDB 10.11
container, using only synthetic schemas. The wrapper's fixed
`currency_coin_test` schema name was required by its loader isolation guard;
the first UUID-named run passed coin tests but correctly refused that loader
stage. The corrected full run passed and removed its test schema. No production
configuration or player data was used. GitHub CI is not the acceptance gate.

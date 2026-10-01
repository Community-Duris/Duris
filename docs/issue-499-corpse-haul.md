# Issue #499: corpse haul count-limit notice

| Issue | Root cause | Branch | Validation | Merge |
| --- | --- | --- | --- | --- |
| [#499](https://github.com/Community-Duris/Duris/issues/499) | `select_bulk_get_item` appended the same count-cap rejection for every rejected direct child while correctly continuing the container scan for money. | `codex/issue-499-corpse-haul`, [PR #502](https://github.com/Community-Duris/Duris/pull/502) | Sanitizer-backed production-function regressions; disposable MariaDB and flatfile game journeys below. | [cb69396c](https://github.com/Community-Duris/Duris/commit/cb69396c273e2e46efab2fbd77da6b7527d86739) |

## Redacted command/result evidence

The disposable mini-world placed three visible ordinary roots (banana, ruby, sapphire) and 3 silver in Raoul's corpse. The player carried 11/11 markers before `get all corpse`. On the original master, the real MariaDB server returned `Haul: 3s` followed by **three** identical `You can't carry any more.` lines. The production-function regression likewise failed its one-notice assertion before the fix.

With the fix, `get all corpse` at 11/11 returned `Haul: 3s` and **one** count-cap line. MariaDB ownership showed zero of those three roots player-owned, all three remained visible in the corpse, and the persisted wallet was `[0, 3, 0, 0]`. The flatfile inspector reported the same custody and wallet result.

After `drop marker`, `get all corpse` returned `Haul: a sapphire` and one count-cap line for the other two roots. Both backends showed exactly one of the three roots player-owned after save and reconnect. A later death and `get all Taverek` on the player's corpse filled the inventory, listed only acquired roots, and again printed one count-cap line. MariaDB credited the remaining 3 silver in that haul. Flatfile reported a separate coin-transaction admission refusal after the committed item batch; the coin remained in the corpse, and `get coins Taverek` at the item cap then credited exactly 3 silver. This distinct refusal was preserved in the output.

The focused harness also covers NPC and player corpse flags, a later coin acknowledgement, a malformed bounded sibling cycle, a hidden direct child, a nested descendant, source movement/removal, interrupted and duplicate completions, disconnect, and strict NPC publication. The new flag records only the typed count-cap condition; other rejection text is untouched.

## Local verification

- `make -C src` with `PERSISTENCE_BACKEND=mariadb` and `PERSISTENCE_BACKEND=flatfile` (GCC 12, C++20).
- `./scripts/format.sh --check` and `git diff --check`.
- `python3 tests/async/test_corpse_haul.py`, `test_bulk_get_publication.py`, `test_money_carry_count.py`, `test_bulk_get_source_selection.py`, `test_coin_get_completion.py`, and `test_get_all_durable_chain.py`.
- `python3 tests/async/run_corpse_haul_count_cap_journey.py bin/server/dms_new mariadb` against a disposable MariaDB schema.
- `python3 tests/async/run_corpse_haul_count_cap_journey.py bin/server/dms_new flatfile` against a disposable flatfile authority root.

The MariaDB schema, flatfile authority root, runtime area edits, account, and character are test-local and removed by the journey. No production state is changed.

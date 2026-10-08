# Plan 5: native legacy auction receipt publication - 2026-10-07

The independent auction decoder now preserves native version-1 receipt
publication semantics. The original native reader treats legacy receipts as
already published; the independent observer previously returned false. Existing
aggregate money reports do not use this flag, so this closes a decoder/observer
disagreement rather than proving event delivery or a complete auction audit.

## Branch, ownership and exact source

- Local/remote branch: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Branch base: `013e1e3d069ee303347965a0163ce8ed4e95b07d`.
- Solved-issue code commit: `d7c60d3c39da335abf997c49bfff291db0a00177`.
- Tested primary base: `bf1aaad3786015b121112b23c5ee587929105de9`.
- Final immutable test tree: `d2b62a01917df43898ffaeda4585245aef41eb05`.
- Archive SHA256: `9e8538afef17bf621f4e855ef00aa6422d87ff5adbf5043de75078790f74a088`.
- Native tree: `833d3085815b396861ad18a77635412212381e4b`.
- Canonical-64 migration tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.

Only these three code/test blobs are overlaid on the frozen primary export:

| Owned file | Tested/result Git blob |
| --- | --- |
| `scripts/qualify_flatfile_native_auction.h` | `78536cf49a7da6bedf8935a3601ecca08e872a72` |
| `tests/async/flatfile_auction_money_cases.py` | `e4e87d2debcbeb5504a0ff2964b948fcbd2e2edf` |
| `tests/async/flatfile_auction_money_fixture.cpp` | `e44415cc9ae92093dd7eb33cb997b5a8acc61eac` |

The branch retains native tree `4abb609524a1f1682ea4c190f82d75003c4d679b`
and migration tree `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`. Bare branch HEAD
is not the combined candidate. Qualification belongs to the immutable primary
composition above, with the explicit compiler-source proposals below. Earlier
0055 evidence is not used to qualify canonical 64. No other branch is created
or switched; all seven earlier remote tips remain ancestors.

## Established defect and complete fix

`src/flatfile/flatfile_auction_repository.c` in the tested primary initializes
`event_published` to `version == catalog_legacy_version`. The independent
`decode_catalog` initialized it to false for both versions. A legacy file has
no publication byte, so the independent observer could never match native true.

The existing differential test now includes each receipt's publication flag in
both native and pure output. The exact pre-fix tree
`0a9984fdb366e103482a2b0ce432a27c5386e15b` reproduces native `[true]` versus pure
`[false]` on the same accepted version-1 file and unchanged authority. Its archive
SHA256 is `04ded6a9e90b6bac3ad11e18412508d954ecf23c7950a74fa0f57ec811395e94`.

The final test tree differs from that control only in the auction header's
legacy default and explanatory comment. Version 2 continues reading its explicit
boolean byte and refusing nonboolean values. Explicit assertions cover legacy
true, current false and current true. All original cases remain. The C++ fixture
also receives the three preexisting formatter changes already present in the
primary; no native implementation or file format changes.

## Exact execution and results

Host launch commands from the Plan5 worktree, with TEMP/TMP on `D:/Dev/Temp`:

```text
python -X utf8 D:/Dev/Temp/accounting-plan5-auction-receipt/run.py red
python -X utf8 D:/Dev/Temp/accounting-plan5-auction-receipt/run.py red-providers
python -X utf8 D:/Dev/Temp/accounting-plan5-auction-receipt/run.py red-complete-providers
python -X utf8 D:/Dev/Temp/accounting-plan5-auction-receipt/run.py green
```

Every run invokes the complete existing entry point through `runpy`:

```text
python3 tests/async/flatfile_auction_money_cases.py --native-source /workspace --artifacts /workspace/bin/tests/auction-receipt-<stage>
```

The pinned offline, read-only container image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with GCC 13.3.0 and Python 3.12.3, 2 CPUs and 4 GiB. Native metadata fixtures run
in RAM to preserve their original private filesystem requirements. Compiler
outputs go directly to
`D:/Dev/Builds/Duris/accounting-plan5-auction-receipt-20261007/<stage>/bin`;
RAM symlinks preserve the original runner paths. Compilation caches are disabled.
All original compiler flags, sanitizer options, assertions and per-command
timeouts remain. The harness declares one 900-second entry budget, never reset.

- `red`: original shared fixture fails linking missing retirement/native-cost/
  native-give symbols before the receipt test. Exit 1; not qualification.
- `red-providers`: those three real providers resolve their symbols, then link
  exposes missing `auction_native_command_decode`. Exit 1; not qualification.
- `red-complete-providers`: real native providers link; the original entry reaches
  the precise legacy publication mismatch. Exit 1; defect control.
- `green`: full entry passes **117 cases, zero skips**. These comprise 77 native
  decoder comparisons (12 accepted, 65 refused) and 40 other observations,
  including money comparisons, prior-history omission control, security,
  journals/locks, cooperative budgets and inactive authority. Every observation
  reports an unchanged complete retained inventory. Total observer time,
  including source guards and collection, is **254.251958 seconds**;
  265 commands are retained. No assertion, original case or timeout is removed.

The economic fixture and independent executable retain their ASan/UBSan flags.
The original native oracle and operator retain their original compiler recipes
and flags. All four compile freshly; no old ELF is substituted. Final ELF pins:

| Executable | SHA256 |
| --- | --- |
| economic_fixture | `ad35efe49d5d072dbf6cce2d83fbdf1e3e4c99902279b52904b1739ed9d2c3a2` |
| independent | `79f54bbcad396b4f244b64b8658d631f9b94d689625d91ffc6e6ae4f42ce751a` |
| native_fixture | `1189d12a2534d48e0adcba935dc16fc0f15161ee001dde1d99ea0bd85fc04de9` |
| operator | `88a2ff816e9ab32b3801e48ec6f09174f025a81a26a852f037895c7152c640aa` |

Full-file clang-format 18.1.3 fixpoint checks and Python AST parsing pass, as does
`git diff --check`. Formatting uses pinned image
`sha256:f87358723e903ec1b3ac9d28d78117fbc10487dccb7f034438591d91d90ca39c`.
This slice changes standalone operator/test code, with no `src/` change and no
new production server build. No SQL database or game server starts. Both-engine
SQL and genuine producer/gameplay qualification are not inferred from this run.

## Narrow shared recipe handoff

No shared API, schema or native implementation change is requested. The tested
primary's restore builder already has its reviewed three-provider repair; the
operator compiles through that exact unmodified 62-source recipe. Two test
recipes still need current provider closure:

1. `tests/async/test_flatfile_accounting_store.py:SOURCES` omits
   `src/item/lockpick_retirement_continuation.c`,
   `src/economy/native_quest_cost.c`, and
   `src/economy/native_quest_coin_give.c`. The original
   `test_flatfile_restore_economic_authority.build_fixture` consumes this list
   and also links `auction_command.c`, whose current decode requires
   `src/economy/auction_native_command_context.c`. That non-GC fixture also needs
   the real `auction_native_expected_player_forest` definition. The disposable
   compile uses the exact native body through the primary's existing
   `test_auction_retained_seller_fee.py` extraction recipe. Its byte interval,
   owner/recipe hashes and translation-unit hash are in
   `native-forest-source.json`. It is not a stub or rewritten implementation.
2. `flatfile_auction_money_cases.build_native_oracle` consumes the shared restore
   source list and needs the same native command-context source. Its original
   GC flags discard the unused bind/forest path, so no forest body is added to
   that oracle.

The disposable compiler commands append exactly those real inputs before `-o`.
`commands.json` retains both original and executed argv; the seal verifies that
removing only those inputs and undoing the output path restores every original
argument in order. No native file, shared maintained recipe, macro, sanitizer,
deadline, assertion, source node or accounting contract is edited or replaced.
Consequently this is a full component pass with recorded provider proposals,
not a claim that both unchanged maintained test recipes now link. Primary owns
their adoption and must repeat the full original entry with zero proposals;
the common accounting-store suite should also run with its repaired recipe.

## Evidence, curator packet and remaining gates

Evidence root: `D:/Dev/Tests/Duris/accounting-plan5/auction-receipt-20261007/`.
Each stage contains source archive/manifest, Docker argv and terminal state,
complete compiler/test argv, log, observer result, original native metadata
inventory and copied scratch. The successful driver's detailed report is
`green/scratch/tests/auction-receipt-green/evidence.json`.

Pre/post source fences authenticate every original body, mode and link in both
defect-control and successful compositions. All runs are terminal, with no OOM.
POSIX read-only verification authenticates copied native bodies and link
targets: 873 regular files/four links in the defect control, 826/four in the
successful run. Original modes/owners stay in native inventories; NTFS copies
do not qualify those attributes. The seal uses extended Windows paths without
resolving or following native copied links.

`seal/evidence.json`, the additive remote follow-up and post-push
`delivery/result.json` form the curator-ready packet for the nonblocking
primary-local notebook. No notebook application, acknowledgement, cross-chat
notification or primary integration of this slice is claimed.

Shared recipe adoption remains a concrete handoff. Full physical/current-money
and UID/history reconciliation (including auction item literals and NPC/treasury
coverage), native V2 install/recapture, genuine producer/gameplay/fault/load
journeys, authentic retention/erasure continuity, both-engine upgrades and the
primary's tested combined Plans/R1-R8 release candidate remain open. Synthetic
fixtures and this passing component do not close those gates. Accounting is not
activated; wallet-root exclusions and the declined inactive spell-path change
are untouched. No deployment, merge, production mutation or audit correction
occurs. Continued independent Plan5 work is not blocked by notebook upkeep.

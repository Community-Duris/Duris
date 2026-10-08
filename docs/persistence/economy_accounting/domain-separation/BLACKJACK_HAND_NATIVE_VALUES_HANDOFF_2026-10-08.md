# Blackjack Hand native values acceptance — 2026-10-08

The independent hand-score/ownership slice selected by the accepted
[gambling authority boundary map](GAMBLING_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md)
passes 67 controls at `-O1` and the same 67 at `-Og`. Both profiles compile the
complete genuine `src/economy/cardgames.c` translation unit and link its actual
`Hand::BlackjackValue()` with the genuine public Card/Hand header operations.
This qualifies those operations on the selected legal inputs, including their
ownership transfer and destruction sequence. It does not qualify gambling
admission, native publication, recovery or the complete translation unit's runtime
behavior. No production file changed, and no additional provider or stub was needed.

## Frozen inputs and owned files

The source export is public primary
`78d71393ee625a975b66d583b447577267e3616c`. Its Plan 5 correction is documentation
wording only: one native table has five physical columns, while a reader selects
six fields including image length. Its source and async-test trees match the
previously accepted Collector candidate; that closed acceptance was not rerun.

| Input | Identity |
| --- | --- |
| `src` tree | `833d3085815b396861ad18a77635412212381e4b` |
| `tests/async` tree | `790f367adf805a69d53aac6460938f5c921f9136` |
| `source.tar`, 43,520,000 bytes | SHA256 `a0888954829f7790cdd0670c6647ef00d5ee03b4fc8d3ac9074250d82cbe81b4` |
| `src/economy/cardgames.c`, 23,201 bytes | Git blob `ecc5178fdbea9e739fbf30ff613ad93e4f0f2bfb` |
| Same complete provider | SHA256 `8ce1d4fe622ce3193d9d88d335bb5ce3062d3a8ba371626b9894612f839b450a` |
| `src/economy/cardgames.h`, 4,692 bytes | Git blob `0e92f3ee29b04da68ef0757cd6fac75de074183f` |
| Same public header | SHA256 `e00e761462ffc2375a5af976d314ec672e8f5e136cc626baa8b8613d7270eeeb` |
| Final Python runner | Git blob `63adb9030af4b7b2235a3e02dfc839d4bf0812b0` |
| Final Python runner | SHA256 `de5cc960629622e2e1dd4c37824ef816e6c5f1f526c06bfd371311bf04491ab6` |
| Final C++ harness | Git blob `34b8db0d74a8ce755721050a97c2dc260e894485` |
| Final C++ harness | SHA256 `3ed17544159fe840ff7ed86934bc1e8cfe57509ca563d90d070c9c4a02d6f439` |

The only owned paths are this handoff and
[`test_blackjack_hand_native_values.py`](../../../../tests/async/test_blackjack_hand_native_values.py)
and [`blackjack_hand_native_values_harness.cpp`](../../../../tests/async/blackjack_hand_native_values_harness.cpp).
The working branch starts at the accepted map commit
`d5cd32e827e28dabef6d8878d510a7c3565e86a7`.
The private archive manifest authenticates every one of its 2,816 original files
by byte count, SHA256 and Git blob. Both retained exports preserve those files;
only the two reserved new tests are added. The final export and copied build
driver match the owned tests byte for byte. A separate Windows Git check compares
the manifest's complete original path/blob set with the exact public revision.

## Manual cases and ownership controls

The harness uses a literal 52-entry single-card score oracle, indexed by legal
Card ID. It writes the rank expectations explicitly for each suit, rather than
calculating a second scoring algorithm. Empty plus all 52 legal single cards and
the following 14 selected multi-card scenarios give 67 separate process cases.
Every listed prefix has a manually written expected score and count.

| Case | Card IDs in receipt order | Prefix score expectations |
| --- | --- | --- |
| `ace_nine_ten` | 1, 9, 10 | 11, 20, 20 |
| `ace_ten` | 1, 10 | 11, 21 |
| `two_aces` | 1, 14 | 11, 12 |
| `two_aces_nine` | 1, 14, 9 | 11, 12, 21 |
| `two_aces_ten` | 1, 14, 10 | 11, 12, 12 |
| `four_aces_nine` | 1, 14, 27, 40, 9 | 11, 12, 13, 14, 13 |
| `bust` | 10, 23, 5 | 10, 20, 25 |
| `five_cards` | 2, 3, 4, 5, 6 | 2, 5, 9, 14, 20 |
| `soft_seventeen` | 1, 6, 10 | 11, 17, 17 |
| `hard_ace` | 10, 9, 1 | 10, 19, 20 |
| `order_ten_ace_nine` | 10, 1, 9 | 10, 21, 20 |
| `order_nine_ten_ace` | 9, 10, 1 | 9, 19, 20 |
| `four_suits_two` | 2, 15, 28, 41 | 2, 4, 6, 8 |
| `three_faces` | 11, 25, 52 | 10, 20, 30 |

Each case creates actual Cards with legal IDs through the public constructor and
an actual default Hand, asserting distinct IDs within that selected hand and
distinct live allocation addresses. Every observation repeats the actual score,
count and null-owner checks three times. A second actual Hand independently owns
Cards 3 and 4 with manual score 7/count 2 throughout source mutations and transfer.
These hands represent separate ownership fixtures, not a shared game deck.

After every receipt, the original hand is checked against its manual prefix.
`Fold()` must return the original known head, leave score/count zero, and return
null on a second fold. Known allocations from the relinquished list are attached
to a fresh actual Hand solely through `ReceiveCard()`. The harness never reads
or writes card links or protected fields. It checks all destination prefixes,
the still-empty source and the independent hand. It destroys the source before
observing the destination again, then adds Card 13 to the independent hand
(manual score 17/count 3) and checks the destination unchanged. The destination's
genuine destructor frees the transferred Cards; only nonowning tracked addresses
are cleared afterward. The independent hand's genuine destructor frees its own
allocations. Empty exercises the same sequence without allocations to transfer.

There is no class-access trick, copied scorer, raw-byte capture, synthetic
participant, replacement provider, Deck fixture, actor identity fixture or
accepting currency/authority/event stub. Actual production anchors are
`cardgames.c:71` (score) and `cardgames.h:24,63,74,85,86,92,98` (constructor,
default Hand, count, owner, receive, fold and destructor). Non-null player-owner
binding, invalid-ID clamping, arbitrary deck permutations and gambling outcomes
are outside these selected controls.

## Compile, link and execution evidence

Ubuntu-22.04 WSL uses Ubuntu `g++-12` 12.3.0-1ubuntu1~22.04.3. The resolved compiler
SHA256 is `88315fd2d961a1f4e4070b104d7bd4c7670df19d9949409879ba269466c196d3`.
Both profiles compile the whole provider and the driver separately with C++20,
`-Wall -Wextra -Wpedantic -Werror`, `-g`, AddressSanitizer/UndefinedBehaviorSanitizer,
frame pointers, function/data sections, `-fno-pie` and `-D__NO_MYSQL__`. Linking uses
`-no-pie`, `--gc-sections` and a retained link map. There are no added link libraries
or extra production translation units beyond the standard compiler/sanitizer
defaults. Resolved shared libraries include ASan, UBSan, libstdc++, libm, libgcc,
libc and the ELF loader; their actual bytes are pinned separately for each profile.

`nm` confirms the score's executable definition. Its code-symbol filter rejects
table, dealer callback, cards-object, Deck, display, currency, authority, RNG and
event function definitions. The link map independently places the blackjack
table, shuffle, dealer event and cards-object text sections in discarded input
sections. Compiling their source does not mean those bodies execute.

ASan module registration retains some function-qualified static data even when
the associated executable body is discarded, including table coin values/lock,
the cards-object Deck pointer/guard and display buffers. `retained-provider-data.json`
discloses the entries matching the excluded provider-name filter; the full symbol
dump also retains other static data such as suit/rank strings. That JSON is not
a complete globals census. These are data, not live table/Deck/display functions;
ASan registration itself remains live. No Deck is constructed by the harness.

| Fresh final profile | Exact PASS processes | Dependency inputs | Wrapper seconds | ELF SHA256 |
| --- | --- | --- | --- | --- |
| `-O1` | 67/67 | 371 | 12.348551527 | `b17843be8218313bc1dae1cc2decb6fef733548f7b2f04b540c780c80f14c827` |
| `-Og` | 67/67 | 371 | 11.744837070 | `d408c4595ea81e733987a282215d58e5bb05e98d21832ee315a697ab81f78cae` |

Compile and link logs are empty under the strict warning flags. Every runtime log
must equal its one expected PASS line, including
`genuine_hand_score_ownership=1 gambling_native_publication=0`; extra diagnostics
fail acceptance. Leak detection and ASan halt-on-error are enabled; UBSan halts
with stack traces. Neither profile emitted sanitizer diagnostics.

All external commands have retained argv, deadlines, status and elapsed times:
compiler/tool queries and symbol/library inspection 20 seconds each, each compile
900 seconds, link 120 seconds, each case 30 seconds. The private outer runner has
a 3,600-second deadline. Each profile records 77 successful bounded commands.
Nonzero command output is preserved. Timeout output is preserved with a marker
and return code 124. Independent controls of the exact final runner's logging
helper pass for exit 7 with both output streams and a 0.2-second timeout with
partial output. These controls do not execute gambling code.

The initial `acceptance-O1` build compiled and linked successfully, then failed
its overly broad substring symbol assertion on `blackjack_table(`: ASan retained
function-qualified static data. No runtime case ran in that failed attempt.
The failed export, objects, ELF, map, logs and command metadata remain intact.
Only the runner's symbol classification was corrected to inspect executable
symbol types and separately disclose retained data, with explicit discarded-text
checks. No provider, harness, stub or library was added to fix that check.
The fresh final exports/build directories supplied all reported passes.

## Reproduction and retained evidence

Private evidence root: `D:\Dev\Temp\blackjack-hand-native-values-20261008`.
Build root: `D:\Dev\Builds\Duris\blackjack-hand-native-values-20261008\bin`.
`source.tar` and `input-pins.json` authenticate the frozen export;
`candidate` retains the failed-check inputs and `candidate-final` the corrected
runner. `prepare.py`, `prepare_final.py` and `qualify.py` retain exact preparation
and wrapper commands. Existing build directories are never reused or overwritten.

From Ubuntu-22.04, a fresh profile can be reproduced with a new build directory
below the same D: build root (substitute `Og` for the second profile):

```bash
base=/mnt/d/Dev/Temp/blackjack-hand-native-values-20261008
build=/mnt/d/Dev/Builds/Duris/blackjack-hand-native-values-20261008/bin
CXX=g++-12 TMPDIR="$base/compiler-temp" BIN_ROOT="$build" \
  timeout 3600s python3 "$base/candidate-final/tests/async/test_blackjack_hand_native_values.py" \
  --source-root "$base/candidate-final" --optimization O1 \
  --build-dir "$build/reproduction-O1-$(date +%s)"
```

The final `acceptance-O1-final` and `acceptance-Og-final` directories retain driver,
objects, GCC dependency files, all 67 case logs/command records, runtime profile,
compiler/helper/tool identities, direct/dependency/shared-library hashes, ELF,
symbol dump, link map and artifact manifest. After execution, the runner rechecks
all dependency, tool and shared-library hashes. Private `verify_evidence.py`
independently rechecks both original exports, final inputs, every profile artifact,
exact cases/logs, command statuses, dependency identities and preserved failure.
`qualification.json`, `git-input-review.json`, `final-review.json`,
`publication.json` and the final `evidence-index.json` retain their results.
The final index hashes retained scratch/build evidence, including the failed
attempt; export contents are authenticated through their separate manifest.

Python syntax parsing and repository clang-format 18.1.8 checking of the sole new
C++ file pass. The staged diff passes whitespace checks and contains exactly the
three reserved paths. There is no whole-server build, broad suite, database,
migration, server, native publisher, journal or gameplay execution in this slice.

## Remaining authority and native journey limits

The accepted map's source findings remain: the early active guard refuses every
blackjack invocation; no double-down or natural 3:2 payout exists in this provider;
OFFER's actor is not bound to the later Hand owner; canonical-value debit and
single-denomination stake posting need a compatible policy; C++ loss/interruption
reasons 45/46 exceed the published SQL constraint 1..42; and object/Hand pointers
are not a durable round identity. This acceptance supplies none of those missing
contracts and enables no wagering route.

Original round admission/source identity, retained authenticated participant and
table/current-world ownership, frozen RNG/cards/configuration, SQL/flat transaction
ownership, save exclusion, original native publication/guarded ACK and a genuine
cold recovery fixture still require their owners' work and actual journeys.
Reported unavailable private-primary/exact439 native inputs were not executed or
adopted here. The original continuing Goal remains **BLOCKED and unfinished**.
This independent score/ownership acceptance does not resume or complete it, nor
does it close gambling, implementation, release or native qualification gates.

# Required baseline controls: primary integration

The retained-root audit cannot discover missing controls for initialized books
with no roots. The independent operator now pages required books from the
authenticated epoch catalogue, including inactive epochs. Removing every book
file therefore leaves a detectable requirement.

`flatfile_economic_audit.py --scope baseline-controls` selects at most two books
per bucket and checks each head, all 16 reservation shards and every original
root/witness referenced by reservations. Existing independent decoders are
reused. The one-shot audit retains its full consecutive history, root-count and
terminal checks. Pages preserve sticky findings and safe, source-bound progress;
refusal rotates without advancing a cursor or earning a completed range.

The six source/test imports are exact Plan 5 `7962dd8ad` inputs, completing the
combined candidate described in [page consistency qualification](
PLAN5_PAGE_CONSISTENCY_PRIMARY_INTEGRATION_2026-10-07.md). No shared producer,
coordinator, schema, writer registry, activation or inactive-path change.
Existing manual central entries run the original suites with their fresh owned
artifact directories. The operator documentation supplies the new scope.

## Primary qualification

All original native lifecycle, authority and baseline-marker suites pass once,
zero skips, under their original providers/flags/sanitizers/deadlines and
900-second central limits. Lifecycle passes 131 original cases plus 46 receipt
pages and 35 baseline controls; authority passes 20 healthy stores/367 refusals,
28 root/29 authority pages, 1,058 metadata/574 envelope comparisons and 30
baseline controls; markers pass 69 original controls. Exact elapsed times are
321.021, 338.947 and 212.051 seconds respectively.

The full raw source archive is
`b5edc0a5f78aa86104c1825862df4172675c7e34135257d634f10f33993686b8`.
All 6,364 regular files retain bytes/modes throughout; four explicit pinned
documentation/help aliases are excluded. Primary authenticates the source,
14 evidence files, 11,879 native entries, 11,839 regular-file hashes/modes and
nine actual executable hashes. The original qualification seal is
`a9dd6151c7124bf3b08c162ae4f6114d1e4b5e7e2ea43d523150c471611648f5`.
The exact stopped owned container is removed and absence verified, no volumes.

After qualification, the three maintained C++ inputs receive clang-format-18.
Full lexical comparisons preserve every code character, literal and comment;
native evidence remains bound to the raw archive, not a claimed rebuild of
formatted inputs. All three Python inputs stay byte-identical to the native
candidate. Formatting-equivalence records follow:

- `scripts/qualify_flatfile_economic_baseline.h`: native `e0ce6b32a7862b04b28a05c1309bd2f1ee63f938f8ab7d0781e82d2c59e4a44f`, maintained `6e2b14761e76bb707424111c6688969be65a8e2b15073c87947e6e16c971a7f9`; 12308 lexical elements identical.
- `scripts/qualify_flatfile_economic_records.h`: native `5ab788e17ec23fd3e4144d2c2020c3ba463ee029fd4fc37c3031b37f494cf844`, maintained `547fb99e4c1484c230e971dddfd4e96f14f0cad22ba5d6df0b1995621d7bad5e`; 29337 lexical elements identical.
- `scripts/qualify_flatfile_restore.cpp`: native `3f899feb951e074b25df1d2fd19188bc02c63bef1e61c4cb5932e73ffb49d1f6`, maintained `3f899feb951e074b25df1d2fd19188bc02c63bef1e61c4cb5932e73ffb49d1f6`; 17959 lexical elements identical.

Private packets: `tmp/plan5-required-controls-primary-integration-20261007`,
`tmp/plan5-required-controls-native-primary-20261007` and
`tmp/plan5-required-controls-format-primary-20261007`. Normal accounting
contracts, checkpoint unit checks, formatting and whitespace validation pass.
Unrelated SHOP/restore-report WIP remains exact and unstaged.

## Boundaries

Control/reference pages do not prove complete empty-root history, total head
root count, orphan or unknown-initialization namespaces, or current native
holdings. Budgets remain 16,384 reads/128 MiB/8,192 directory entries/30 seconds
with the existing 45-second subprocess cap; other scopes retain their budgets.
All complete/closure/release flags remain false. Actual producer/player
journeys, growing histories, populated upgrade, full restore/fault recovery,
operational policy and mixed workloads remain release gates. No full Plan or
R1-R8 completion is claimed.

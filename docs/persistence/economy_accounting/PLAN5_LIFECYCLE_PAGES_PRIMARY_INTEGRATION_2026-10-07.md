# Required lifecycle receipt paging: primary integration

The independent reader now checks required lifecycle receipts in durable,
bounded pages. Previously only the one-shot lifecycle audit could inspect this
scope. Pages derive their census from actual unique initializer roots in the
validated lifecycle catalog, including inactive epochs, so missing receipt files
remain detectable. Existing native receipt authentication is reused unchanged.

Six source/test files are imported from Plan 5 commit
`e9d498f655b81b8947709914a0ddcc35a6a4559d`, over maintained parent
`66a3deee3641631b071c727194fdb5d22f146478`. This slice changes independent
audit/qualification tools; it changes no producer, writer registry, receipt
format, production activation or inactive-accounting behavior.

## Behavior and boundaries

`flatfile_economic_audit.py --scope lifecycle-receipts` uses a separate progress
format and checkpoint outside authority. Each page checks at most two required
initializer operation IDs in one rotating bucket. A fixed historical ceiling
allows progress while later arrivals are revisited in subsequent ranges.
Missing, unsafe or corrupted receipts/witnesses are attributed to the required
initializer ID. Refused buckets rotate without advancing their cursors or
earning completed ranges; findings remain sticky.

Lifecycle pages retain the qualified 16,384-read/128-MiB/8,192-directory-entry/
30-second budget. Existing root and authority pages retain 64 reads/32 MiB.
Checkpoint locking, safe atomic replacement and timestamp refusal remain.
An updated qualifier/reader requires a new separate source-bound checkpoint.
One-shot audit semantics and native receipt bytes remain unchanged.

Coverage remains explicitly partial: complete, consistent-entire-sweep, release,
native holdings, baseline book, orphan namespace and lifecycle closure flags
remain false. A completed historical range is not full accounting qualification.

## Primary native qualification

Pinned offline image:
`sha256:4994cc50a09a4acd40462ff412f3c3df21fc7c23a3f298f7fa725f0e2a350fb3`.
Raw source archive:
`bcf22cb873240cc3cc0e26200e7bbbabadad7c0d0bdf12a22b33a037f619f56f`.
Integration seal:
`e7907bea121c4971880002c44a518c31d39127461bb7c6a59e5a048e042da546`.
All 2,884 protected source files retain exact bytes/modes before and after both
original suites. Unrelated SHOP edits and the incoming untracked restore report
are excluded and preserved.

Run the unchanged original entrypoints on Linux with native dependencies:

```sh
python3 -B tests/async/test_flatfile_restore_lifecycle_receipts.py \
  --artifacts /absolute/checkout/bin/tests/fresh-lifecycle-artifacts
python3 -B tests/async/test_flatfile_restore_economic_authority.py
```

Both run once with existing flags, providers, assertions, internal deadlines,
sanitizers and 900-second central timeouts; cache is off, network is disabled.
The lifecycle suite passes 131 original cases (nine accepted, 122 refused) and
46 new page controls with zero skips, in 249.876 seconds. The authority suite
passes 20 positive stores, 367 corruption refusals, 28 root-page and 29
authority-page controls, including 1,058 metadata and 574 envelope comparisons,
in 288.194 seconds. This broader suite also covers the shared segment preload
change in root-page decoding. No assertion, budget or refusal is waived.

Private reproducible packet:
`tmp/plan5-lifecycle-pages-native-primary-20261007/`.
`QUALIFICATION.json` SHA256:
`d7d1cafafa451caaa2dce018b9b5bb9a42382ad66156d9db4221779d06d98d91`.
`evidence/RESULTS.json` SHA256:
`fbc940d698f9aab3255e9e41e66813d4f6607fd447394b22ff1e5fb418a746c5`.
Native archive SHA256:
`83d7ba26dd1f90b16afae568ef273a8caad01ffa447552ce9d6b4123e03fa3a6`.
Primary independently verifies all 12 evidence files, 5,867 native artifact
entries, 5,849 regular-file hashes/modes and all six actual executable hashes
against the original reports. The read-only authority ELF observer preserves
the three binaries before the original temporary-directory cleanup; it never
reads or changes fixture state.

The first clang-format-18 check refused three formatting sites in the incoming
records header. After native qualification, only that maintained header is
formatted. A retained before/after lexical comparison proves all 25,602 code
characters/tokens, comments and literals identical, with no whitespace-sensitive
literal or comment changes. Native evidence remains bound to the unformatted
archive; its binaries are not claimed to have been built from the formatted
file. Maintained header SHA256 is
`73624da1f036e0eeb1ce0d50ae273731b2d1f227104e579179c27f9f6133e765`;
incoming/native header SHA256 is
`6b905460cf7adb4a39be876ea9c9983d66cacb734d9d44c2109613bb232f40e8`.
The other five maintained source files are byte-identical to the qualified
imports. Formatting/normal validator/whitespace checks pass.

Owned stopped container identity and exited-zero state are verified before
exact-name removal; absence is verified afterward. No volumes, SQL services,
ports, game connections or production mounts are created. Separate cleanup
receipt SHA256:
`2b9a58a5a3652d44c746664bace4cea12ce70f12c8e56e61331c8d3b843a12b9`.

## Remaining qualification

This is independent-reader and modeled native-codec evidence. Complete native
holdings, genuine producer/player journeys, growing retained-history workloads,
full restore/fault recovery, operational policy and release qualification remain
open. Existing manual central entries stay appropriate for the required fresh
owned artifact directories. No Plan 5 or R1–R8 completion is claimed.

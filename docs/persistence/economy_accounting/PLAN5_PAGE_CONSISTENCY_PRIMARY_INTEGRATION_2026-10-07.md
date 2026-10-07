# Flatfile page consistency: primary integration

A semantic finding previously left `consistent_page=true` when the native
page returned normally. The operator now requires both no refusal and no
current-page findings. A later healthy page may be locally consistent while
earlier findings remain sticky and the CLI still exits 1.

This issue imports the exact operator change and strengthened original root,
authority and lifecycle assertions from Plan 5 commit `b5fcd8434`, over primary
parent `aa252cd8134972548a953a0c5e500da2f8d06af7`. No producer, native evidence
format, inactive-accounting behavior or activation gate changes.

## Qualification

The primary qualified the combined consistency and required-baseline-controls
candidate once using all three original native entrypoints. This batch covers
these exact consistency branches and assertions; it is not a claim that the
intermediate consistency-only commit was separately built. The subsequent
baseline-controls integration completes the qualified candidate.

Pinned offline image `4994cc50a09a4acd40462ff412f3c3df21fc7c23a3f298f7fa725f0e2a350fb3`.
Source archive `b5edc0a5f78aa86104c1825862df4172675c7e34135257d634f10f33993686b8`;
source manifest `e6441590a4e7d5c74c53b03ca53931fc306a91e87a9af00a6657d06b855fdb87`.
All 6,364 regular source files retain bytes and modes. Four documentation/help
aliases are explicitly excluded and pinned; no native provider is excluded.

- Lifecycle: 131 original cases (9 accepted, 122 refused), 46 receipt-page
  controls and 35 baseline controls; 321.021 seconds.
- Authority: 20 healthy stores, 367 refusals, 28 root and 29 authority pages,
  1,058 metadata and 574 envelope comparisons, 30 baseline controls;
  338.947 seconds.
- Markers: 69 original controls (6 qualified, 7 readable but unqualified,
  56 refused); 212.051 seconds.

All passed with zero skips. Original providers, flags, sanitizer settings,
internal deadlines and 900-second central limits remain. No cache, networking,
SQL/Redis/game services, server rebuild, activation or production data.

Private packet: `tmp/plan5-required-controls-native-primary-20261007`.
Original results seal `6ad9c58c21c9f0cc4f22cae71b9683261a65ba70feb8af65f1d07b4d9f419a7e`;
qualification `a9dd6151c7124bf3b08c162ae4f6114d1e4b5e7e2ea43d523150c471611648f5`;
native archive `f8f58f10b5aba37e2a0deb3b7052311cd7a136e1ddd664cb26edfa0357226d29`.
Primary independently authenticated all 14 evidence files, 11,879 native
entries, 11,839 regular-file hashes/modes and nine actual executable hashes.
Exact exited-zero container cleanup and subsequent absence are verified;
no volumes were removed. Unrelated SHOP edits and restore-report WIP remain.

## Limits

This proves independent offline reader behavior on modeled native-codec
fixtures. Complete holdings, orphan/retained-history closure, actual producer
and player journeys, populated upgrade, full restore/fault recovery, operational
policy, mixed workloads and release remain open. All complete/release flags
remain false. No whole Plan or R1-R8 completion is claimed.

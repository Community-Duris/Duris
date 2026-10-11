# Preserve native-birth sanitizer refusal while fixing warnings — 2026-10-11

The actual common-retention and craft SQL-header regression builds exposed two
strict compiler errors after genuine native-birth companions were linked.
ASan excludes the bounded decoder's sole call to its internal build helper;
`[[maybe_unused]]` now describes that conditional use. The accounting compiler's
unsupported-profile branch explicitly marks its command argument unused.
All profile guards, function bodies, errors, Source arithmetic, warning flags,
sanitizers and inactive behavior remain unchanged. Unsupported profiles continue
to return unresolved. Changed-line clang-format and independent Source review pass.

Fresh GNU 13.3.0 qualification against 6d0542847 passes six invocations building
eight objects: the two actual production targets through make for MariaDB and
flatfile, and each target under strict ASan/UBSan for both backends. Evidence:
`tmp/native-birth-warning-fix-v3-root-20261011/native/RESULTS.json`, SHA256
b5b92b57251d89603f75ed1f9abf64933fca0d69b8b17abb1ec05587df2b4a7d.
This is compiler/object qualification; it does not establish full server links,
original common/craft runner passes, gameplay, persistence or recovery journeys.

The registry refresh changes two existing Source pins and genuine affected line
coordinates only; 484 pins total, all 482 unowned pins and 931 writer policies
preserved. Census remains 2,911. Prior frozen HRT failures remain failures;
later combined-source builds and regressions remain required. Accounting stays
inactive/CLOSED, coverage incomplete and release BLOCKED. R1–R8 and full original
implementation, architecture, Plan5 and release acceptance remain open.

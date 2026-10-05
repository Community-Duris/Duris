# Flat baseline marker recovery fixture — 2026-10-05

The unchanged native flatfile owner compiled, then aborted at its old
`changes.size() == 17` assertion. Current production initialization already
stages 19 after-images: sixteen reservation buckets, the complete book head,
the durable epoch initialization marker and its authenticated authority control.
The existing [marker contract](BASELINE_INITIALIZATION_MARKER_INTERFACE_V2.md)
requires those files to publish atomically.

Only `tests/async/flatfile_accounting_baseline_test.cpp` changes. Assert exactly
19 images and each `epochs.eae` / `authority.eal` filename once; extend original
initialization crash coverage from 17 to all 19 operation boundaries. After
recovery, identical original initialization must return `already_exists` with
empty output, matching the existing implementation. Preserve every original
case, the 20 batch after-images, sanitizer/allocation hooks, compiler flags and
the original 300-second native / 600-second outer limits. No production behavior
or authority gate changes. Read-only review and changed-line formatting passed.

## Executed original native owner

The corrected original recipe compiled and executed successfully with ASan and
UBSan in 357.646 seconds inside the original outer bound. It passed:

- before-journal, after-journal and all 19 initialization after-image recoveries;
- before-journal, after-journal and all 20 baseline batch after-image recoveries;
- 626 staging and 421 lookup allocation-failure cases;
- exact replay, cross-batch duplicate refusal, epoch isolation, coherent corrupt
  receipt/witness refusal, maximum witness and native-sentinel preservation.

| Retained artifact | SHA256 |
| --- | --- |
| Corrected native binary | `51c2c139fb0c357773c2984ca1d765e0ca9d6b628bfb7b35ed7471e51b1f760a` |
| Original owner stdout | `f2278c5671c195c693b284dfa9abc386811236e6ac398299dc9c19a9457babb2` |
| Corrected fixture | `756d5a169f1671dc9549acdd23fe82c51874a1cd6ed6407887f373ecf4a470d6` |
| Frozen source manifest | `f7bfa7a6956dad9f84e61349f2f0d70cbff7f9faf82e2af280a068eba0acb563` |

The exact owned tools runner was removed after final source verification.
Private evidence remains in
`bin/tests/plan1-flat-baseline-marker-tools-20261005-77a00a02e766`.
The preceding failed attempt and native binary are preserved separately.

## Remaining scope

This run uses the frozen EAB2/schema61 candidate over `b754e2962`, the separately
published collector recipe, and this one fixture overlay. Pending native/schema
sources are not installed by this commit. The original baseline preparation
SQL/client-free modes separately passed in the
[collector linkage milestone](BASELINE_COMPONENT_COLLECTOR_LINKAGE_2026-10-05.md).
The two results prove their respective original component owners; they do not
prove all Plan 1 gates, gameplay writers or maintained current public server builds.

Both maintained candidate server builds are in progress. Independent audit
exception repair and the genuine historical C05/populated upgrade remain open.
The coherent original C05 source needs its actual original cutover/coordinator/
journal providers; its first 15-unit link failure is retained. No stubs or relaxed
flags are used in the 18-unit successor. Complete Plan 1, R1–R8, coverage and
release remain unproven. Current inactive behavior and activation gates stay.

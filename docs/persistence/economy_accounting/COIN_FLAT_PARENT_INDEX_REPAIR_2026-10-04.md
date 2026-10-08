# Flat coin recovery signed parent comparison repair — 2026-10-04

Plan5 reports the actual maintained flatfile C++20 build failure in
[its frozen build handoff](PLAN5_V3_PHYSICAL_RECOVERY_BUILD_HANDOFF_2026-10-04.md).
On f8b8dec31 source, 475 of476 translation units completed; the sole remaining
error is `int32_t parent_index == size_t index` under maintained `-Werror`.
No server was produced or managed v3 cold restore executed.

Primary changes only that condition in `observe_and_project_flat`: require a
nonnegative parent index, then compare its size_t representation to the original
vector index. Root sentinel -1 cannot match a child. Valid child comparisons,
4096-object/depth/graph bounds, duplicate/foreign custody/native literal checks,
retained original command and publication/ACK fences are unchanged. There is no
schema, activation or inactive gameplay policy change.

Original raw input SHA256: a64e8f47c814aeb34c11ea3e6051f2527b3d2b80da5963a9a0175b149d30b8ae.
Candidate raw input SHA256: 0670e71865ebe3543ec18c4142653ed0ea6499cd93cc3bb36f06407f951e8397.
Independent source review confirms this narrow semantic equivalence. The peer's
copied translation-unit strict syntax diagnostic passes for the same condition;
it is diagnostic evidence only. Primary changed-line clang18 formatting reaches
fixed point; all48 candidate raw pins and normal source-inventory metadata are
verified separately. Census completeness and executable coverage remain false;
release remains BLOCKED. No new native build, regression or gameplay runs are
claimed; both backend builds and actual retained-pile publication/cold replay,
managed v3 backup/restore and complete candidate qualification remain at their
original major-plan batches.

The original failed build and its evidence are preserved, including peer475
objects and335.406-second failure. The corrected source clears a known compiler
condition; it does not establish current-candidate native/recovery acceptance.

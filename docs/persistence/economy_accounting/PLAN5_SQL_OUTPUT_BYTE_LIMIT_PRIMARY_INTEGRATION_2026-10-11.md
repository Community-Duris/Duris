# SQL audit output byte-limit primary integration — 2026-10-11

The current exporter accepted a compact JSON body at the input limit, then added a newline that made the emitted file too large for the existing reconciler. The primary now checks the complete encoded output before exclusive file creation. Oversized output returns the existing refusal code without creating a file; existing output remains unchanged. Connection cleanup still occurs before this check.

The completed narrow fix from Plan5 commit `5afb2af41337bc387b79b2d6956827e3eaa5c798` is adopted without replacing its independently evolving exporter. Only two lines change in the current production exporter. All34 other function/class ASTs and all original test ASTs are preserved. SQL queries, transaction semantics, partial-evidence declarations, schemas and authority remain unchanged.

The existing `plan5_recent_uid_scope` registration adds the new case while retaining its original case, offline provider, engines, mode, timeout, coverage and environment. The regression crosses four body sizes around the limit with absent/existing output (eight controls), checking bytes, JSON, no creation on refusal, connection closure and preservation. Linux executes the original conditional0600 assertion.

## Verified evidence

- Original source with the new regression: four real failures at/above the body limit; below-limit controls pass. The first attempt was additionally blocked by sandbox temporary-directory permissions; its log is preserved separately and is not the defect reproduction. A workspace-scoped disposable temporary directory resolved that environment boundary.
- Corrected current code: all14 UID/export tests pass on Windows.
- Frozen corrected component: all14 tests pass on Linux; both registered cases pass directly; the original matrix runner lists the row with two required cases.
- Existing481 source pins remain registered, with only the exporter, test and integration-manifest pins updated; all478 others authenticate against disk and Git. All931 writer policies,2911 census entries and2853 mapped sites remain unchanged. The regenerated matrix changes only those three candidate pins. Coverage remains incomplete and release BLOCKED.

Component source starts at `83e2e4392064667fc4f64780183d17b5acc431ef` plus the three exact owned files. Freeze SHA256: `56c87b793e11da9c4896578109023347af39c6bbd58329fb7e5f84d6cdaeddf9`. Exporter SHA256: `ac7299ecf327bd36d3882fa3406bfd351fe1f6a4723f2018f9dae2b5eb8f81ea`. Test SHA256: `60b13011a72c00e478af099b7ec033b9db0d30f9535c662df66527d05bc8d924`. Registration SHA256: `80d930fdf21908f56c9bdb5023ade03a1083b6a9e8f40de3c1b8c02998d14292`. Linux result SHA256: `01832ecd4ad83c885b429cdc2722f046c6c9a7d8c8066c224a896390a16cfef5`. All frozen bytes remain unchanged after execution.

Published remote documentation-only descendant `6022c0ad7feab34c13e27738d4295b65b8d010ac` was adopted before this milestone; its two changed documents leave the tested implementation identical. Private evidence remains at `tmp/plan5-output-limit-adoption-root-20261011/`, including original preimages, upstream reports, regression failures, exact commands/results, component inventory and source/pin refresh. The milestone's Git parent/result binds publication.

This is exporter output-protocol acceptance. SQL capture and client connection are mocked while the real CLI and file operations execute. It does not qualify native SQL authority, gameplay, backup/restore, recovery, the full accounting candidate or release. Remaining Plan5 room/custody/operator work stays independently owned and incomplete. Accounting remains inactive; admission stays CLOSED; no production or live-game operation occurs.

The separate [combined build checkpoint](GUARDED_COMBINED_BUILD_CHECKPOINT_2026-10-11.md) qualifies its exact earlier native revision only. No C/C++ source changes occur in this exporter fix.

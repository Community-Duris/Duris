# Current61 deletion inspector linkage and original recovery qualification

The deletion inspector recipe now links the actual baseline adapter and codec.
Its original 75-source prefix remains exact; only those two providers are appended.
No production implementation, assertions, cases, sanitizer flags or runtime
deadlines changed. This fixes unresolved baseline symbols after the EAB2/schema61
integration. The native inspector and all three original flatfile recovery
journeys pass on the current maintained production source.

## Original inspector

`DURIS_TEST_SANITIZERS=1 python3 -B -u tests/async/test_flatfile_character_delete.py`
exits0 in470.428998 seconds. ASan/UBSan covers corrupt refusal/repair and all18
erasure journal boundaries; character and account deletion pass. Stderr is empty.
The actual 77-source recipe SHA256 is
`214f158599e693f8c2f9acda358ffe6dbf7d1b236e728f2703e990dee83b8a15`.
Inspector binary SHA256:
`0b9e6aab52ce7232fd8cece9910a2fefe626e1c2fbf172103d85316789225897`.
Stdout SHA256:
`0614a518765ea84350798b45f9f10081e4e10dcc8e15ade1bef438c1d501f356`.

Evidence is retained under
`bin/tests/flat-delete-inspector-current61-directory-native-linux-20261005-e64a41e90dd7`.
Its3,087 declared source inputs are unchanged; source pins are
`4a06617d67c2a97689957c0ff3d2e8d14d5b39aedd0afed8a28f1a6857838dad`.
Ten regular artifacts export with exact hashes before owned cleanup.

The subsequent journey attempt in that same packet fails before gameplay because
`/suite/lib` is absent. Its fresh helper compile passes in137.444 seconds, but
the journey runner exits1 after163.599719 seconds. That failure remains evidence;
it is not counted as a retention pass. An earlier packet also failed before
native execution because the artifact parent directory was missing.

## Original three journeys

The successor supplies2,655 exact tracked public HEAD inputs from `lib`,
`areas_mini` and `areas`, totaling165,241,866 bytes. Two relative area-help
symlinks retain their actual tracked targets and stay inside the source root.
All original3,087 code/test/migration inputs remain byte-identical. Declared
source pins:
`8dfd2bed2360f55eb200b302c5c8372924e40128d625b7124113c5db34108633`.

The unchanged command is
`python3 -B -u tests/async/run_plan5_retention_journeys.py --server bin/server/flatfile/dms_new --backend flatfile --inspector bin/tests/flatfile-character-delete-inspector`.
It exits0 in350.177339 seconds with empty stderr. Only the verified inspector
binary above is reused; the successful inspector execution belongs to its
original component attempt. The real restore-authority helper compiles afresh
in140.402 seconds with zero object reuse.

| Original journey | Actual menu | Captures | Cold restarts | Result |
| --- | --- | --- | --- | --- |
| Character deletion | Yes | 3 | 1 | PASS |
| Durable-fence failure and recovery | Yes | 5 | 3 | PASS |
| Uncertain publication and recovery | Yes | 5 | 3 | PASS |

The original checks cover missing-authority refusal, preserved snapshots and
quest aliases, playable retry, deletion once, alias erasure, usable account after
restart, pending journal recovery, durable quest-state refusal, uncertain
publication, persistent refusal and non-cancellable original requests. Every
journey preserves the original two epochs, two claims and four retained roots.

Evidence is retained under
`bin/tests/flat-retention-public-fixture-native-linux-20261005-179374bd82b3`.
Journey stdout SHA256:
`6198096c077b534b40aa1d59751cb14caf54f2d7c6fef229ec38dda64fcb7907`.
Twenty-one regular artifacts export with exact hashes; the owned runner and both
volumes are removed after verified export. All5,742 source bytes and modes remain
unchanged. Both attempts use the same pinned compiler/dependency image,
read-only source, isolated fresh state, network-none container, two CPUs and3GiB.
No private local data or environment file is included.

The reused maintained server SHA256 is
`c437f8d3fc761b5dc8e1a284c097d464622b893c9bed64b821c5fd1bd2ad5e70`.
All1,521 production inputs still match its verified build. Public base HEAD is
`f899ef4862f61652e7cce4707bb6f7c137ca39cb`; this milestone changes only the recipe
and owning evidence. Plan5's historical canonical56 results remain separately
scoped; these results qualify the actual current61 shared inspector and journeys.

## Remaining scope

These are real native menus and restarts with structurally seeded historical
roots and inactive accounting. They do not prove active monetary/item producer
coverage, complete native/world capture, populated full-service restoration,
retention/erasure/export policy, full R8 or release qualification. Those original
requirements remain open. Accounting activation, declined inactive spell-path
work and production data remain untouched.

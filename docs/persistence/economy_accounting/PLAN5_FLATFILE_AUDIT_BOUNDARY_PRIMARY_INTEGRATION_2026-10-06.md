# Primary integration: read-only flatfile audit boundaries

Date: 2026-10-06. Maintained parent: `a455dcb94df3e9be8cabf3487e4cc5eda3afa816`.
Exact peer issue: `ece7a6280`.

The operator audit previously read native economic files without acquiring the
writer's authority lock or bounding aggregate work. It now takes a nonblocking
shared lock on the same existing inode used exclusively by native writers. It
authenticates private directories and lock ownership, refuses pending transaction
journals and rechecks directory/lock identity before publishing its result.
It never creates authority, repairs evidence or runs journal recovery. An absent
lock remains acceptable only for an empty legacy observation.

Only the operator branch installs a cooperative30-second,128-MiB/2048-read/
8192-directory-entry budget. Repeated physical reads and ignored entries count.
Offline restore candidates retain their original qualification and recovery rules.
RAII releases descriptors on both success and refusal. The operator guide records
these limits and their cooperative scope; no durable pagination or hard I/O
latency guarantee is claimed.

The complete seven-file peer import includes the five operator C++/header inputs,
native fixture and original Python authority regression. Independent source/AST
review finds no blocking regression or missing predecessor. Original helper
functions and original candidate/corruption blocks remain exact; the original
main statements retain their order and assertions, with added boundary/budget
controls and the deliberately extended sanitized reader.

Primary runs the complete original native regression against a frozen maintained
parent plus these seven exact files. All2881 selected source bytes/modes remain
unchanged. The disposable container has no network, ports, production-data mounts,
SQL service or live-game connection. Original compiler/sanitizer flags and
per-case deadlines remain unchanged.

`python3 -B tests/async/test_flatfile_restore_economic_authority.py` exits0 in
416.748 seconds:

- All12 authority-boundary cases produce their expected exit and preserve evidence.
- All nine budget/shared-reader/replacement controls pass.
- Original1058 metadata and574 envelope comparisons pass;11 retained records and
  ten malformed-envelope refusals remain covered.
- Original20 positive stores,367 corruption refusals,50 generic semantic
  corruptions and54 native semantic decodes pass; economic evidence is unchanged.
- Original retained baseline-book, cross-epoch/source-claim and sanitized-reader
  checks pass.

Frozen archive SHA256:
`e4b89d0db64f1a43d34e96d64b5483165009f1bdce24827a493caefaaa1d3c0b`.
Native log SHA256:
`7a0d3f72baa977aceeeba4a66ffa86294cb6a1f5af40998061eaf1469eb8bb93`.
Protected primary packet:
`bin/tests/plan5-flatfile-audit-boundary-primary-integration-20261006/`.
Exact imports/preimages:
`tmp/plan5-flatfile-audit-boundary-primary-integration-20261006/`.
All six changed C++ inputs pass changed-line clang-format18. Normal accounting
validation and whitespace checks pass. The [peer qualification](PLAN5_FLATFILE_AUDIT_BOUNDARY_QUALIFICATION_2026-10-06.md)
retains its separate frozen-source scope.

This closes the read-only operator locking/work-bound gap. It does not qualify
the private753-provider producer candidate, genuine cold-world recovery, complete
native holdings, every writer, activation, or full R7/R8 release acceptance.

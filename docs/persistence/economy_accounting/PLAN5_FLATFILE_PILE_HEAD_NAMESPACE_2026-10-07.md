# Plan 5: durable pile-head namespace audit - 2026-10-07

The physical namespace reader previously classified native pile-head EPH1 files
as ignored. With a private authentic native head changed from EPH1 to EPH2 and
its checksum recomputed, the unchanged reader reports zero findings and
known_physical_economic_namespace_closed=true. The real native pile-state API
rejects that file. This could hide damaged durable accounting state from the
operator's captured namespace traversal.

The independent reader now validates pile-head-<16 lowercase hex UID>.eph:
exact133-byte body, EPH1 magic and SHA256, filename/payload UID, current lineage,
catalogue epoch, nonzero revision and operation, native0..INT32_MAX denominations,
retirement byte0/1 and zero retired balance. Reserved malformed pile-head names
and .eph files produce the existing invalid-file finding. Positive files have
family=pile_head. The paired Python family allowlist accepts this owned field
value; the existing source/executable-bound checkpoint refuses mixed readers.
No accounting contract/schema/native API change is requested. A baseline head
can name its preparation ID, so this check does not falsely require a direct
retained command root. Current native custody/owner balance agreement remains
a distinct incomplete acceptance gate.

Branch/worktree: codex/accounting-plan5, `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`. Base `3a80aa8647535f72504c5c232bbe42d520ace563`;
result is the containing commit, recorded explicitly in the external delivery.
Owned files: `scripts/qualify_flatfile_economic_records.h`, `scripts/flatfile_namespace_audit.py`, `tests/async/flatfile_namespace_cases.py`, `tests/async/flatfile_restore_authority_fixture.cpp`, `tests/async/test_flatfile_restore_economic_authority.py`, `docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`, this report, and the
additive remote follow-up. No shared coordinator, writer registry/matrix,
accounting contract, producer, migration, shared build recipe or activation file
is changed. The private native fixture creates heads through the original native
API; retirement follows a committed live baseline rather than bypassing its guard.

Exact tested owned tree `811e12d24e713585521c591d8c4065e488f9a97a`; source archive SHA256
`11414efcbbab82b2ecc74f256916ebb6332347a273bd30a2e8142963b2981240`. Native tree `4abb609524a1f1682ea4c190f82d75003c4d679b` and migrations
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` remain unchanged. All Git archive bodies, canonical modes
and four links are checked through actual terminal completion. Publication
matches those inputs except this report and the follow-up prose.

Final negative command: `python tmp/plan5/run-pile-head-evidence.py red 5`.
It executes the exact frozen old-reader archive with the original private-file
umask0077. The expected version failure is observed; its wrapper exits0 only
after asserting the old reader's false closure. Final focused command:
`python tmp/plan5/run-pile-head-evidence.py green 3`. Final maintained operator
command: `python tmp/plan5/run-pile-head-operator.py green 2`. Both invoke the
complete existing check_namespace_pages function, without selecting away prior
controls, using the original native fixture with AddressSanitizer and
UndefinedBehaviorSanitizer. Each passes73 observations and zero skips:
22 corrupt framing/checksum/identity/epoch/value/retirement/name cases, two
healthy traversals including full unsigned width, and all retained prior
reverse-link, budget, protected-resume, refused-output, interrupted-publication,
closed-revisit and8193-entry inventory controls. Native state remains unchanged.
The original script builder's maintained.build(work/'original-operator') recipe
passes with unchanged SOURCES and original strict C++20 flags. No provider
addition, stub, flag waiver, assertion waiver or controlled link is used in the
final operator run. Its exact g++ command and dependency closure are retained.
GCC's relative include aliases are retained and normalized to guarded repository
paths before Git-body comparison; duplicate canonical inputs must agree exactly.
The first seal attempt stopped at that alias lookup and is preserved separately.
The second stopped when Windows attempted to dereference a deliberately unsafe
Linux symlink preserved by the negative controls. Four such artifact links are
retained by target/target-byte SHA256 through a read-only Linux mount, with no
file omitted. Extended Windows paths authenticate all long native filenames.
The initial link-collector long-path failure is also preserved separately.

Earlier attempts remain preserved and qualified individually: red01 stopped
at a generated audit.cpp filename prerequisite; red02 correctly rejected an
attempted retired baseline; red03 reproduced false closure but its dependency
observer failed on a compiler-profile probe; red04 lacked the original umask;
green01 exposed that same private-mode context error; green02 reached the healthy
sanitizer traversal after22 cases but correctly refused its foreign executable
checkpoint; operator01 built the unchanged original tool successfully then
stopped at an obsolete expected-link-failure assertion. None is relabeled as
the final passing run. No observation timeout was treated as process completion.

Actual primary native oracle command: `python tmp/plan5/run-pile-head-current.py`.
It compiles and executes the actual `e6e058515f5433a1a3028f80d5d7d672d48b2471` pile API plus an independent
owned reader, both with strict C++20/ASan/UBSan. Three authentic retained heads
are checked exactly: UID255 active, UID256 retired at revision8, and UINT64_MAX
active at revisionUINT64_MAX. Nonretired balances are1/2/3/INT32_MAX; retired
balance is zero; lineage1, epoch50 and preparation/retirement operations9/10
match. The original quiescent native read API's transaction-recovery prerequisite
is used only as a private validation oracle; no native API is imported into the
production independent reader. Retained NTFS copies regain documented private
modes before execution, and every file body/mode remains unchanged afterward.
All35 tracked native/owned compiler inputs match the corresponding
primary Git bodies or frozen owned source. Primary native tree
`d149afce4392056ee92afdde2c42dfd22880a5c3`, migration tree `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`,
archive SHA256 `13e90d1ceb9edbfd33e7f2fe3389ce2ae4fa8abcbb54f32365a5c13f90c1a2ab` remain exact. This isolated
wire/API agreement does not qualify the entire combined source or schema.

`python tmp/plan5/run-pile-head-make.py` runs maintained `make -C src -j2
BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile` with retained output
paths from its original build command. All740 object/dependency records and
the server remain byte-identical after matching1291 native dependency inputs
and the original build source. Valid incremental timestamps are restored only
after those comparisons. Zero compiler invocations are observed; no fresh
server build is claimed. The audit header and fixture changes are outside that
unchanged server dependency closure. It also passes57 writer coverage/route
contract tests with zero skips, the ordinary validator, and the expected
release-validator refusal: writer has no executable evidence. Corrected WSL
Git-worktree routing runs scripts/format.sh --check and observes terminal0:
Formatting OK. Staged whitespace passes.

Backend for this defect is private disposable flatfile authority. SQL checks
are not applicable to EPH1 decoding; SQL code/schema is unchanged. Both-engine
current canonical fresh/upgrade, authentic producer/gameplay/ACK/cold recovery
and complete R7/R8 remain required for release. The ordinary server's older
native/schema base is not promoted to the current primary combined candidate.

Evidence under D:/CodexEvidence/accounting-plan5/bin/: pile-head-red-05-20261007,
pile-head-green-03-20261007, pile-head-operator-02-20261007,
pile-head-native-current-01-20261007, pile-head-make-01-20261007,
pile-head-format-01-20261007 and pile-head-refresh-01-20261007. Failed/prior
attempts red01..04, green01..02 and operator01 are retained separately.
Commands, logs, native binaries, generated oracle source, authentic native files,
all dependency probes and actual host/container terminal records are retained.
Seal `D:\CodexEvidence\accounting-plan5\bin\pile-head-seal-03-20261007\evidence.json`; SHA256 `8706cad2a5863e67f16f32778de825db236341556df3ca7413b60ee97a7c74a0`; 24769 artifacts /
3876437215 bytes. The exact publication/remote SHA is in delivery.

No new shared interface request is needed. The earlier standalone-provider
handoff is not a blocker for this final exact owned build, which passes unchanged;
other primary-owned test recipe requests still need qualification on their exact
source. Full native pile/custody/owner literals, treasury, UID/reference/provenance,
source/history coverage, release-host backup/restore/retention, real journeys and
the tested combined candidate remain open. Inventory and synthetic fixtures do
not establish release completion. All seven earlier tips remain ancestors on
the same remote codex/accounting-plan5. The primary's locally maintained notebook
is nonblocking; this report, follow-up, seal and delivery are its curator packet.
Notebook application, notification and acknowledgement are unclaimed.
Accounting inactivity, wallet-root exclusions and the declined inactive spell
change remain preserved. No audit correction or production action occurs.

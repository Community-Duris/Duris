# Plan 5: native locker literal and custody audit - 2026-10-07

The existing operator could decode custody and world catalogs but had no locker
literal comparison command. Four before/after cuts establish the missing command
on real native-readable DURLOCK V1/V2 catalogs and its repaired behavior. The new
independent reader compares durable locker item literals with custody, preserves
all original custody/world checks, and makes no full-release claim.

Branch/worktree: `codex/accounting-plan5`,
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `02ba8688468c6905058d52d6f0fd927add518d3e`. The containing commit is the
result; external delivery records exact local/remote SHAs and verifies the seven
previous tips remain ancestors. No branch switch occurred. Only the requested
remote branch receives this slice; experimental-accounting remains primary-owned.

Owned files: scripts/qualify_flatfile_native_locker.h (new),
scripts/qualify_flatfile_restore.cpp, tests/async/flatfile_custody_audit_fixture.cpp,
tests/async/test_flatfile_custody_audit.py, AUDIT_OPERATIONS.md, this report and the
additive PLAN5_REMOTE_FOLLOWUP_2026-10-06.md. No shared native provider, contract,
coordinator, migration, registry/matrix, activation owner or runner was edited.
No API/schema field change is requested.

Exact final tested owned tree `0c39313e83b98eb20cab538d886a6dd50d0e8af0`,
archive SHA256 `aaf89ddb9318cd8702c57cbc4d7b3b7d39e893bf82acb60403ec562faeb14835`.
Native tree `4abb609524a1f1682ea4c190f82d75003c4d679b` and migrations
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` are unchanged older owned source;
they do not qualify the current combined candidate. The publication has the same
code/compiler inputs, adding only completed lifecycle/locker documentation.

Actual native oracle commit `911e5789f8a18186181f78fafef9b4fd0155369d`,
whole tree `78fd5962ad0bdec7ce631b5f61138cff0a53f811`, native tree
`833d3085815b396861ad18a77635412212381e4b`, migrations
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`, archive SHA256
`6aabfae3198cc8bc855cb16b10d692863551edcfc12fe2bc3709ef28e519ae3d`.
Fresh publication refresh reached `d98f67e6c657cd39d24617ef8fb93dc3b058469f`:
native/migration trees remain identical; only primary documentation changed.
Latest finish, remaining requirements, review checkpoint and shared native
qualification bodies are retained as immutable Git bytes. Earlier required
AGENTS/README/Plan 5 and raw source reads remain in the lifecycle seal; tracked
AI_CONTEXT.md is absent. Console decoding does not replace original raw bytes.

The CLI is
`--economic-locker-custody-audit /absolute/private/state-root [--limit 0..100]`.
It invokes no native repository, mutation, journal recovery or correction logic.
It uses the existing protected shared read lock, checks pending journals before
and after observation, validates private regular files without following links,
and retains the aggregate 128 MiB/30 s audit budget. Unsafe/corrupt inputs refuse
with the fixed error and no partial JSON. An absent catalog is explicitly absent
and unverified. Complete finding totals remain invariant under detail limits;
UID details are decimal strings. Names, passwords, policy bytes and item text are
omitted from output.

The reader validates complete DURLOCK V1/V2 envelopes, body length/checksum/EOF,
nonzero catalog revision, locker/chest/access bounds, strictly ordered positive
locker and chest IDs, global chest uniqueness, unique canonical names, and exactly
one positive player/association/account owner. Account-owned V2 lockers require
the exact side/name/racewar relationship. Historical signed race fields remain
unconstrained when the native format leaves them so. Public/private policy framing
and exactly one public chest per locker are checked without exporting policies.
Access pairs are ordered, unique, revisioned and tied to existing locker names.
Item forests reuse the independent complete item decoder, preserving native
limits, positive globally unique UIDs/vnums and locker equipment value -1.

Each chest maps to custody owner kind5, locker ID and chest context. The audit
checks admitted UID, active state, owner/context, root/parent, vnum and custody
equipment zero. When retained inline coin bytes exist, detached framing changes
only the parent index; all remaining literal bytes must agree. Negative first-four
money denominations produce findings even when custody bytes agree. Historical
absent inline payloads remain legacy inputs, without inventing source admission.
Catalog, locker, chest, custody owner and item revisions are independent clocks;
the reader does not equate them. Other owners are counted and explicitly
uncompared. Native holdings, command/source admission, item history, full R7/R8
and release remain false/unqualified in every report.

Pinned native image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC13.3/C++20. The fixture links nine actual providers: flatfile item repository,
authority transaction, store, player snapshot codec, item transfer command,
critical command, economic source event, world item repository and locker
repository. Original strict warnings, ASan/UBSan, no PIE, function/data sections
and GC flags remain. The unchanged maintained operator builder retains its own
ordinary recipe. Native locker reads occur only in private initialized fixture
roots; the operator imports no such provider. Accepted format cases reencode
complete native item fields and agree with the independent decoder.

The initial frozen tree `3cf3bd9523e56ef7caf0a41f2b9b8f4f1713d076` passes all
133 original custody cases (50 accepted/83 refused), 13 custody controls, all59
world formats (13 accepted/46 refused), 37 world finding cases at three limits
and14 world controls, plus87 locker formats,37 locker findings and14 controls.
A copied privacy assertion initially checked world aliases. The final tree
corrects the actual locker identities/password/literal checks and the finding
log tag. AST comparison establishes only check_locker_catalogs and
check_locker_findings changed; every C++ body and every other test function is
identical. The corrected locker subset passes again:87 formats (14 accepted/
73 refused),37 finding cases at limits0/1/100 (111 cuts),14 controls. Passing
custody/world cases were not repeated for this test-only correction. There are
zero skips. Neither suite encountered an assertion, sanitizer or timeout failure.

Cases cover V1/V2, account/player/association ownership, UINT32/UINT64 boundaries,
historical policy fields, complete binary item fields, every identity/placement
and retained coin field, malformed envelope/order/count/name/policy/forest,
negative native money values, absent/legacy inputs and bounded details. Private
Linux inventories assert bodies/stat/nlink/inode/mtime remain unchanged through
operator reads. The14 controls include held/missing locks, all pending journal
families, public modes, symlink/dangling/hardlink and zero-length files. Copied
NTFS modes do not establish original Linux permission/link observations.

Ordinary maintained reproduction:
`python3 -u -B tests/async/test_flatfile_custody_audit.py --native-source /absolute/exact/native-checkout`.
Retained observers invoke those exact maintained functions with retained output
instead of deleting the temporary evidence. Initial/final process durations are
174.222034676/150.979012301 s, both terminal0; logs SHA256
`34d33024ba349a506e2f33c2a02cc5ed31fda1ec3b10e927fc50f39fc22ebc81` /
`234ad905ea582451da7d1cd231ce8123cc3a85240492319e4e43858098bfec2e`.
Two successful compiler -MM probes bind333 tracked inputs:116 actual-primary and
217 owned. Frozen bodies, canonical Git tar modes0664 and all four tracked links
remain unchanged through terminal completion. All exact compiler/operator/Docker
argv, helpers, original budgets and case reports remain in the packets.

The four before/after cuts use the previous sealed operator binary and final
qualified operator/fixture binaries, with SHA256 bindings checked. For each
native-readable locker version, the old command refuses and the new command
reports three compared items with scoped owner verification and releasefalse.
Authority inventories remain unchanged. No observation budget was restarted.

Repository format/check passes with isolated clang-format18.1.3 and leaves all
four qualified code bodies identical. Incremental native command
`make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/workspace/bin/tests/money-history-build/bin OBJDIR=/workspace/bin/tests/money-history-build/objects DMS_BINARY=/workspace/bin/tests/money-history-build/server`
passes after1,291 native inputs match the previously validated build. All740
objects/.d and server remain identical; no fresh compile or combined build is
claimed. Server SHA256
`f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`.
`python3 -u -B scripts/validate_economy_accounting.py` passes; --release returns1
for `writer has no executable evidence`. This refusal stays a release gate.

Evidence root `D:/CodexEvidence/accounting-plan5/bin/`. Retained candidate-green01,
privacy01, red01, format01, Make01, publication apply/format/refresh01 and the
read-only next-owner discovery packet are sealed. Seal
locker-custody-seal-01-20261007/evidence.json SHA256
`5506cd65f0c4924a0932a13247e2aaf31c501b3bbb92d63d1edd07afef56dc44`
binds3,075 entries/1,199,606,509 bytes. Build binding
locker-custody-build-binding-01-20261007/binding.json SHA256
`9059f613b1ba60083211b8701ce1811a77650bfdb4caf1603da36c1f6249bde9`
rehashes all1,491 earlier regular build bodies/1,294,533,318 bytes. Publication
and delivery separately record final tree/archive/result/remote and ancestry.

Narrow primary handoff: no fields, API or schema change. Register the expanded
original test invocation through the primary-owned maintained runner, preserving
all original custody/world cases, assertions, real providers, flags and budgets;
locker checks are additive. Application/notification is unclaimed. This report,
follow-up, seal and delivery are curator-ready. Primary-local notebook maintenance
remains nonblocking; notebook application/acknowledgement is unclaimed here.

SQL is inapplicable to this flatfile locker component; it is not skipped evidence
for full both-engine qualification. Full native V2 source/fixture export remains
needed. Remaining gates include all native holdings/ledger/history/treasury
owners, original player/persistence/fault journeys, current combined builds,
canonical migration fresh/upgrade/rerun on MySQL/MariaDB, authentic backup/cold
restore/retention and full R1-R8. Older0055 and isolated/catalogue/synthetic passes
do not qualify the combined release. Accounting stays inactive; wallet-root item
exclusions and declined inactive spell behavior remain. No activation, deployment,
merge, production change or audit correction occurred. Full Plan5 remains active.

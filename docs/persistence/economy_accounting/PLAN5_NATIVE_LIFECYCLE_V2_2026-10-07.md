# Plan 5: independent published native lifecycle V2 coverage - 2026-10-07

The previous immutable receipt reader accepted only native DURELR V1 and only
wallet/bank opening witnesses. The reader now implements the primary's published
V2 envelope and retained room-pile pairing/digest contract. The established
envelope refusal is repaired and the independent coverage component is verified.
Full original native V2 receipt/installer qualification remains open: its private
implementation and original fixtures have not been exported. Modeled framing,
native EAB codec passes and historical V1 suites do not close that gate.

Branch/worktree: `codex/accounting-plan5`,
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `2e02e9ce10472320a33cd98239aa9244fb1404b2`. The containing commit is the
result; the external delivery records its exact local/remote SHA and verifies
all seven earlier tips remain ancestors. No branch was switched. Only this
remote branch receives the result; experimental-accounting remains primary-owned.

Owned files: scripts/qualify_flatfile_economic_authority.h,
scripts/qualify_flatfile_economic_lifecycle.h,
tests/async/flatfile_lifecycle_v2_fixture.cpp (new),
tests/async/test_flatfile_lifecycle_v2.py (new), AUDIT_OPERATIONS.md, this report
and the additive PLAN5_REMOTE_FOLLOWUP_2026-10-06.md. No shared coordinator,
accounting contract, producer, migration, registry/matrix, activation owner or
shared runner was edited. No public API/schema field change is requested.

Exact tested owned code tree `e368e0ecc654ca9c9112fc7aa15ffb318978e166`,
archive SHA256 `88feb89dc0722472886234f202af240b225cb0e7abd5a16ec5fda0f59ef3f4c7`. Owned native tree
`4abb609524a1f1682ea4c190f82d75003c4d679b` and migrations `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` are unchanged.
Publication differs from the frozen tested tree only in these three documents.
This older native source is not the combined candidate.

Actual native EAB oracle commit `5826195dd7365ea8e482545de21b770ea0da3a73`,
whole tree `477f8f867c4c3d7f5ad5ac22b7de08e0ba11a5c5`, native tree `833d3085815b396861ad18a77635412212381e4b`,
migrations `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`, archive SHA256
`85d837e9c581a8ccd6711b8bc952894ac11185e18b96ff0d145adfe9d7ecf586`. A later refresh reached
`911e5789f8a18186181f78fafef9b4fd0155369d` with the same native/migration
trees and new SQL/documentation work. Those successor changes are not qualified
by this slice. Immutable Git bodies for current Plan 5, finish, remaining
requirements, checkpoint, coordination, AGENTS/README and the source contract
were captured/read. AI_CONTEXT.md remains absent from tracked source. Console
replacement decoding does not replace preserved raw document bytes.

The published contract names native implementation SHA256
`d81ec1ad1d16a0f797b3c13eed6a655af9f3d90d3b9d86adddf36b12387967a2`,
private combined30 archive
`8c93aae1b7d8bd0896f5263bf9c15aa09b0ca71e7e7844cb7355e4ef982ad7e5`
and manifest `f6707df33a10de7fec99e68b2a233f10308187884d1b1a1cbe56b226fcb9be31`.
These are primary-published identities; the private bodies were not locally
available or executed. They are not authenticated native V2 fixture evidence.

DURELR's original 48-byte header, checksum, exact body length and original
V1 field ordering remain. Native versions 1/2 are allowed; unknown native 3/4
refuse. Qualifier catalogue V2/V3 remains a separate format. V1 accepts no pile
witnesses and keeps the original wallet/bank digest. V2 adds no trailing UID
vector: sorted room piles come from retained EAB1/EAB2 rows. The reader validates
complete witness framing/forest, lineage, exact mapped-account plus item count,
unique account keys, active room owner/state, positive INT32 room, context zero,
root equal UID, parent/equipment zero, nonzero native revision and fingerprint,
and exact matching kind-3 holding revision/source. Four denominations remain
nonnegative INT32. Extra/missing holdings and malformed pairings refuse.

The coverage digest hashes ASCII DURIS-FLATFILE-COVERAGE-V2 without NUL, the
unchanged 32-byte V1 wallet/bank digest, little-endian U64 pile count, then each
increasing UID, room, native revision, four denomination values and 32-byte
source fingerprint. Historical retries use retained rows; current physical
piles are never recaptured. Native revisions remain independent of opening
effect 0->1. Existing command, witness descriptor, original plan, reservation,
receipt and common root proof remain mandatory. This reader invokes no mutation,
native lifecycle codec, recovery or installation path.

The six strict/sanitized before/after envelope cuts establish V1 preservation,
V2 refusal before/acceptance after and V3 refusal in both. Their bodies are
modeled framing controls, not original native V2 receipts. All 65 coverage cases
pass: 12 accepted, 53 independently refused. Accepted witnesses round-trip
through the exact current-primary native EAB codec and match an independent
Python digest oracle. Cases cover EAB1/2, empty V2, wallet/bank/pile-only,
multiple sorted piles, 3,071 paired holdings, full UINT64 UID/revision, INT32
room/denomination boundaries, revision100, every pair field, duplicate/order,
reserved fields, counts, zero fingerprints, truncation/trailing bytes and
unknown versions. Five envelope controls cover versions0..4. Every case checks
authority bodies and original Linux stat observations remain unchanged.
Zero-denomination cases establish parser behavior only; fresh native genesis
admission still refuses zero-value physical piles under the published contract.

The unchanged original historical lifecycle driver passes all131 cases
(9 accepted/122 refused), 46 lifecycle pages,35 baseline controls,69 consecutive
history and48 physical-namespace controls. The unchanged original authority
driver passes20 healthy/367 damaged stores, with 28
money-history, 50 native-domain,
38 wallet/bank, 116
auction-money, 50 auction-source-balance,
61 auction-credit and
58 auction-consumption cases;
its own page/history/namespace/boundary totals remain in the original JSON.
The unchanged original baseline driver passes69 cases:56 structural refusals,
7 readable/unqualified and6 provenance-qualified. All original assertions,
providers, flags and per-command observation budgets remain. No passing native
case was repeated to repair later observer metadata. There are zero test skips.
These historical/component passes do not execute the private native V2 encoder.

Pinned image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC13.3/C++20. The new fixture uses real native baseline codec/adapter and the
original accounting-store provider list plus real lockpick retirement, native
quest cost and native quest coin give; strict warnings, ASan/UBSan, no PIE and
GC flags are retained. Original maintained drivers keep their original build
recipes, including sanitized readers. All Docker/compiler/operator argv and
executed helper bodies are retained. Standalone component reproduction:
`python3 -u -B tests/async/test_flatfile_lifecycle_v2.py --native-source /absolute/exact/native-checkout --artifacts /absolute/fresh-private-directory`.
Original driver commands are recorded in the observer/trace; baseline receives
its required `--native-source /workspace`. The dependency-only completion runs
21 successful -MM probes against the
same archive/generated bodies, binding 85 tracked actual-primary
and 236 tracked owned inputs plus 12
generated compiler inputs. It does not reexecute native cases. Complete frozen
source bodies, canonical Git tar modes0664 and all four links are checked through
terminal completion. The main trace retains 38776 invocations.

Repository changed-line formatting/check passes with isolated clang-format
18.1.3. Incremental native Make command:
`make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/workspace/bin/tests/money-history-build/bin OBJDIR=/workspace/bin/tests/money-history-build/objects DMS_BINARY=/workspace/bin/tests/money-history-build/server`
passes after all1,291 native inputs match prior source. All740 objects/.d bodies
and server remain identical; no fresh compile or combined build is claimed.
Server SHA256 `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`.
`python3 -u -B scripts/validate_economy_accounting.py` passes; --release returns1
for `writer has no executable evidence`. The earlier build is rebound only after
all1,491 regular artifact bodies are rehashed.

Evidence root `D:/CodexEvidence/accounting-plan5/bin/`. Main
lifecycle-v2-green-01-20261007 retains the passing component/lifecycle/authority
results, then closes2 because the outer observer omitted baseline's required
argument. Dedicated lifecycle-v2-baseline-01-20261007 retains its69 passing cases,
then closes1 after an informational compiler search-directory command was
mistakenly parsed as a dependency probe. These are observer errors following
successful assertions, not waived test failures. Dependency-only packet01
repairs that metadata without repeating the cases. Envelope-red01's omitted
probe filename and refresh01's console encoding failure remain retained;
envelope-red02 and refresh02 complete successfully. Formatter01 and Make01
are terminal passes. No timeout was restarted, extended or converted to a pass.

Evidence seal lifecycle-v2-seal-01-20261007/evidence.json SHA256
`7e561ac5a6159755822b092334e8a5b88155b65a862193f0cdb5c047897621e5` binds 43,653 entries/2,789,909,668
bytes including every retained failure and real symlink target read without
following it. Build binding lifecycle-v2-build-binding-01-20261007/binding.json
SHA256 `73db24968bc72e2cac03436e4f06709a40252be689c1f47bfcee0b62ad667414` binds1,491 bodies/1,294,533,318 bytes.
Original Linux permission/link assertions remain original; copied NTFS modes
do not qualify them. Publication/delivery record exact result/tree/archive/remote
and seven-tip ancestry after the push.

Narrow primary handoff: no fields, public API or schema changes. Export the
immutable complete candidate matching the published implementation identity
and original maintained native V2 encode/decode/installer retry/fault fixtures,
historical V1 samples, original provider recipes and budgets. Consumers are this
independent reader and original V2 qualification. Register the new component
command through the primary-owned maintained runner, preserving every original
suite and keeping EAB component proof distinct from native installer proof.
No export, runner application or notification is claimed here.

SQL is inapplicable to this flatfile-only component; full both-engine checks
remain required, not skipped release evidence. Remaining gates include private
native V2 full qualification, all native holdings/ledger/history/treasury owners,
original player/persistence/fault journeys, current combined native builds,
canonical migration fresh/upgrade/rerun on MySQL/MariaDB, authentic backup/cold
restore/retention and fullR1-R8. Older0055 and component/synthetic/inventory passes
do not qualify the combined release. Accounting remains inactive; wallet-root
item exclusions and declined inactive spell-path behavior remain unchanged.
No activation, production modification, correction, deployment or merge occurred.
This report/follow-up/seal/delivery is curator-ready. The primary maintains the
local notebook; notebook application/acknowledgement and cross-chat messaging
are unclaimed and nonblocking for continuing independent Plan5 work.

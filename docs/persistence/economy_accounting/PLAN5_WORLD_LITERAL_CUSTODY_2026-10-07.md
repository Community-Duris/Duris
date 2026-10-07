# Plan 5: independent world literals and custody - 2026-10-07

The previous operator could decode durable custody but could not independently
read native world item literals or reconcile their ownership and full coin
payloads. The new `--economic-world-custody-audit ROOT [--limit 0..100]` checks
DURWRLD v1-v3 and DUROWN v1-v8 under one protected read. Findings never trigger
correction. This closes a world-literal component gap; full holdings, original
journeys, source authority, ledger/history and release remain unqualified.

Branch/worktree: `codex/accounting-plan5`,
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `c35500689dc0f2a3c92d662d30f8711b55a0e938`. The result is the containing
commit; the external delivery records its exact local/remote SHA. All seven
previous tips remain ancestors. No other worktree, shared contract, producer,
coordinator, migration, registry/matrix, activation or shared runner is edited.

Owned files: scripts/qualify_flatfile_native_world.h (new),
scripts/qualify_flatfile_native_custody.h, scripts/qualify_flatfile_restore.cpp,
tests/async/flatfile_custody_audit_fixture.cpp,
tests/async/test_flatfile_custody_audit.py, AUDIT_OPERATIONS.md, this report and
the additive PLAN5_REMOTE_FOLLOWUP_2026-10-06.md. No API/schema change is needed.
A narrow shared test-runner composition request is recorded below.

Exact tested owned code tree `4b9047419a9711f333af4349b0565ea01e8ee1d3`,
archive SHA256 `8fad52fc3e03e68230c164181869f8fdf901f9bd92e1ce28dc5b1c79b9c6efba`.
Owned native tree `4abb609524a1f1682ea4c190f82d75003c4d679b` and migrations
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` are unchanged. Publication adds only
operator guidance, this report and follow-up prose to the tested code. This
older native base is not the combined candidate.

The refreshed actual primary is `439a8fe704b5167171d318f644c3b2b514f8cd22`,
whole tree `1c2c152629724af58aa6f99b592ec4b2d4a10a49`, native tree
`833d3085815b396861ad18a77635412212381e4b`, migrations
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`, archive SHA256
`8da5ce44ae25ea0127c2e7c60c555f20d01eaf0ee510ed32a3998a244ac5562b`.
Its change from 973bb6c is documentation only. Current Plan 5/checkpoint/finish
and native sources were read from immutable Git bodies; the first refresh
attempt's wrong coordination-document path is retained, followed by the
successful fourteen-body capture. AI_CONTEXT.md remains absent from tracked
source. The primary's notebook is explicitly nonblocking.

The independent general item-list decoder preserves every native field as a
borrowed immutable record, with native 4 MiB/4,096-object/8,192-shared-row,
4,096-byte string and depth-32 limits. The existing one-coin parser uses it
under its original 128 KiB bound. Codec-layer unconstrained UID/type/equipment
semantics remain native; the world layer enforces positive UID/vnum,
equipment -1, backward topology and global UID uniqueness.

World decoding validates complete envelopes, checksum/framing, revision,
version-dependent sections, sorted corpse identities, alias consistency and
uniqueness, saved keys, room identities, counts and native metadata bounds.
Corpse cash in v1 is the native default zero; no historical cash observation is
invented. Versions 1/2 omit rooms. Empty saved forests refuse; empty corpse
forests and room forests remain allowed. Complete decoded literal spans stay
alive for the read and are never exposed in diagnostics.

For each world UID, reconciliation checks active custody, owner/context,
root/parent derived from the world forest, vnum and equipment. Available inline
coin payloads compare all retained bytes except the detached parent index,
including unused values, timers, flags, strings, affects and other fields.
Negative first-four coin values produce a finding even when both stores agree.
Legacy custody without inline payload is accepted without granting money/source
admission. Other custody owner families are counted without qualifying them.
The world aggregate revision and custody owner revision are different clocks;
this reader does not equate them or authenticate individual item clocks.

The existing shared read lock, private regular-file checks, aggregate 128 MiB
and 30-second budget and two pending-journal checks protect both reads. No lock
creation, native repository recovery, mutation or correction occurs in the
reader. Empty existing files refuse explicitly. Findings return bounded JSON
and exit 1; totals remain complete at every detail limit. Structural, unsafe
path, unavailable lock and budget refusals return no JSON and the fixed
`native_restore_qualification_failed` diagnostic. Healthy present files may set
`world_owner_literals_verified=true`. Other-owner comparison, complete native
holdings, item history, full R7 and release flags remain false.

The exact final execution is `python3 -u -B /evidence/observer.py` in pinned
image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC 13.3.0. Full Docker argv, observer bodies and every compiler/operator
command are retained. Standalone reproduction:
`python3 -u -B tests/async/test_flatfile_custody_audit.py --native-source /absolute/exact/integrated-checkout`.
The explicit native source must support current quest continuation v6.

Both C++ builds pass strict C++20/-Wall/-Wextra/-Wpedantic/-Werror. The native
fixture uses AddressSanitizer/UndefinedBehaviorSanitizer, no PIE and eight real
providers: item repository, authority transaction, store, snapshot codec,
item command, critical command, economic source event and world repository.
The unchanged maintained operator builder uses its original providers and
flags, without sanitizers. Compiler probes bind 331 tracked inputs: 115 actual
primary and 216 owned. All archive bodies, canonical 0664 modes and four links
are guarded through terminal completion; 612 executed commands are retained.

All 59 world-format cases pass (13 native-accepted, 46 native-refused), including
v1-v3, metadata/order/uniqueness, complete item fields, binary strings, full-width
UID/revision, 4 MiB exact/over, row budget shared across objects, string/object/
depth boundaries, bool/spell/envelope corruption and cash constraints. Native
accepted cases match decoded locations, revisions, money and full item records
through actual native APIs/reencoding. Borrowed recovery-list API plus the
original full-list saved-record API are invoked only in initialized private
native fixtures; the latter's no-op recovery does not enter the audit reader.
An exhausted aggregate byte budget refuses each accepted fixture.

All 37 finding cases pass at limits 0, 1 and 100 (111 cuts). Every tally remains
complete; 101 detail findings truncate at 100. Cases cover missing/unadmitted
UID, custody state, owner/context/root/parent/vnum/equipment mismatch, retained
coin field drift, negative values, legacy empty payload, absent inputs and
other owner families. All 14 world read-only controls pass: healthy/empty/
uninitialized, missing/held lock, three pending journals, public file/root,
symlink/dangling symlink/hardlink and zero file. Authority bodies, modes, kinds
and link counts remain unchanged. The original 133 custody cases (50 accepted,
83 refused) and 13 original boundary controls also pass. Zero skips.

The first owned attempt failed on ambiguous `catalog` type qualification;
the second passed both builds and the old checks but failed on a duplicate
fixture directory label. Both terminal receipts are retained; corrected final
attempt 03 passes the complete suite. No assertion was waived. The first seal
preparation misnamed the native transport's `primary` field as `source`;
its failure is retained and seal 02 verifies the original results without
restarting tests. These are corrected owned/test/evidence defects.

The unchanged authoritative primary command
`python3 -u -B tests/async/test_flatfile_world_item_repository.py` fails at link
before its harness executes. The narrow request to the primary is to append
these four actual sources before -lcrypto in that shared runner:

- src/economy/shop_trade_recovery_manifest.c: native recovery-forest and fee roles.
- src/item/lockpick_retirement_continuation.c: typed retirement-payload validation.
- src/economy/native_quest_cost.c: native quest cost projection encode/decode.
- src/economy/native_quest_coin_give.c: native quest money projection.

Fields/API/schema requested: none. Consumer: shared world-repository runner's
g++ link recipe. Invariants: preserve all ten original providers, strict flags,
original harness assertions and timeouts; do not add garbage collection, fake
providers or activation. The isolated proposal 02 appends only those sources
and passes the entire original harness. It binds 83 exact primary compiler
inputs and keeps all original bodies/modes/links. Proposal 01's wrong source
path is retained as an observer failure. The original shared recipe still
fails; primary application and shared-runner qualification are unclaimed.

Repository changed-line formatting/check passes with isolated clang-format
18.1.3. WSL's initial service error prevented formatting and its argument
sequence also needed repeated --file flags. Docker format attempts 01/02 pass
with exact commands retained; no shared source was touched. Native Make:
`make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/workspace/bin/tests/money-history-build/bin OBJDIR=/workspace/bin/tests/money-history-build/objects DMS_BINARY=/workspace/bin/tests/money-history-build/server`
passes incrementally after all 1,291 inputs match the original source. All 740
objects and .d bodies plus server remain identical; zero fresh compiles are
claimed. Server SHA256
`f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`.
`python3 -u -B scripts/validate_economy_accounting.py` passes; adding --release
returns 1 for `writer has no executable evidence`. Earlier build evidence is
rebound by a new immutable wrapper after rehashing all 1,491 bodies. No current
combined native build is claimed.

Evidence root: `D:/CodexEvidence/accounting-plan5/bin/`. Final owned packet
world-custody-green-03-20261007, native original failure
world-native-current-01-20261007, complete original-harness provider proposal
world-native-providers-proposal-02-20261007, formatter 01/02 and Make 01 are
all terminal. Evidence seal world-custody-seal-02-20261007/evidence.json SHA256
`f88fe295aee2fe317ca4f86914a58d5881cdcd84765efad5b5858fe55215ac7d`
binds 2,558 entries/2,455,476,938 bytes, retaining all failures and actual
symlink targets without following them. Build binding
world-custody-build-binding-01-20261007/binding.json SHA256
`e442e8237a533d862c1e2fcf59b99c68b07e5921004cb7aafcd294b63555eb22`
binds 1,491 earlier build bodies/1,294,533,318 bytes to this seal. Copied NTFS
modes do not replace original Linux permission/link observations. Publication
and delivery packets record exact result/tree/archive/remote and seven-tip
ancestry after the push.

SQL checks are inapplicable to this flatfile-only decoder change; they are not
skipped qualifying evidence for the full release. Remaining gates include all
other native owner families, complete pile value versus postings, UID history,
treasury, original gameplay/persistence journeys, combined native builds,
fresh/upgrade/rerunnable migration and both-engine qualification, authentic
backup/cold restore/retention and full R1-R8 release evidence. Historical 0055
results do not qualify the current canonical combined candidate. Existing
synthetic/component passes and namespace inventories do not close these gates.
Inactive behavior, wallet-root item exclusions and the declined inactive spell
change are preserved. No accounting activation, production data modification,
auto-correction, deployment or merge occurred. This report/follow-up/seal/
delivery are curator-ready; notebook application, receipt acknowledgement and
cross-chat notification are unclaimed and do not block continuing Plan 5.

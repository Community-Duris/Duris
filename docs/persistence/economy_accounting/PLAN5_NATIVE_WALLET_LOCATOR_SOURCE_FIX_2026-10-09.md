# Plan 5 native-wallet locator: source correction; native acceptance pending

## Established defect and source correction

Published source `d0985e90936b02071ae8c107ad37d8c3da58e64f` adds the genuine flat native-mobile wallet
namespace in `flatfile_native_mobile_wallet.h` and the existing authority object.
Its wallet account kind is 1, context is item_owner_type::native_mobile (12),
locator is 7, name is empty, and native ID is nonzero and below UINT64_MAX.
The original independent operator `locator` at
`scripts/qualify_flatfile_economic_authority.h:301` first requires type==kind,
then wallet context==0 and a player-ID limit. It therefore rejects every valid
new native-wallet tuple. This defect is established directly from both exact
published predicates; no compiled/native reproduction is claimed.

The independent reader adds one six-line branch for exactly kind1/context12.
It requires type7, native>0, native<UINT64_MAX and empty name, then returns.
All original player wallet, bank, escrow, claim and treasury rules remain exact;
removing only those six lines reproduces the entire original header byte-for-byte.
Both mapping-row and native-index/key readers already call this same predicate.
No mutation codec/helper is called, include/dependency added, schema or wire format
changed, or authority/activation granted. The bank-only zero-ID key allowance
does not apply to native wallets. There is no new budget or allocation claim.

**Disposition: SOURCE_CORRECTED; NATIVE_ACCEPTANCE_PENDING.**
The source correction is committed for independent review. It is not a native-
qualified solved issue and must not be counted as completed release acceptance.
The current full native execution remains in the original major-plan batch.

Preimage blob: `593272f3ab668e970c0f561deed60cb3ba11a232`.
Preimage raw SHA-256: `f3ac41ffbc026b38b8ed0ef227fab07217828a5c8a4733c01af9b4cf6dc02e52`.
Result blob: `8b0c51679771b4108a635762efa4ef13aac05871`.
Result raw SHA-256: `81a4b93ff4ce8078b8ff30ed77f17952c23c38a9cd173b6c0fd470debc7a8c5d`.
Primary preimage and owned preimage are identical. `source-delta.json` binds the
six-line insertion and complete inverse; every other source body/mode/link is exact.

Owned branch: `codex/accounting-plan5`.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Owned base: `bc6dfa479ddda729505a7857e12b2cae95ef7889`. Result/remote are bound by `delivery/result.json`
after push. Only this header, this report and the additive curator follow-up change.
All 27 preceding owned overlay bodies and seven historical branch tips survive.
No primary/shared coordinator, contract, producer, registry/matrix, migration,
activation or original native fixture/recipe is edited.

## Current frozen sources and actual checks

Primary: `d0985e90936b02071ae8c107ad37d8c3da58e64f`.
Native tree: `5c27cf2e4a5d0ed8aceca3fb231347fcf14ac0a6`.
Migration tree: `a22d54a28286200f09d91d11cb0cbd8c782b0b82`; schema65 inputs unchanged from prior slice.
Before composition: `7963c5db5a1201c56a16015ce2cfcd0845ad0753`.
Before archive: `df1a3f2a5dd42f1ad7d256098ae383162458f61c7344e66e4325d1f3be131540`.
Corrected 28-file composition: `72d467fb4137ae9321470a000f26df5d98f15303`.
Corrected archive: `54937973558385d03f6f85163bb86e78139c9cbaac2f1adaa53cf79ef3fef61d`.
Published complete tree: `6e62e02e55281031fbb5089c79dfb51c2f21a0c4`.
Published archive: `015d459f40adf3c8b5dec765b0cf1bf680d71546f7c3c1b6bb52d58dfb95b0f4`.
Composition 6,755 regular files/four links, published 6,744/four; all regular bodies,
modes and link targets authenticate before/after checks. Neither test composition
imports private candidates, runtime .env, unrelated WIP or production data.

Pinned Python3.12.3 SHA-256:
`e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f`.
Docker image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Both source observers use --network none, --read-only, two CPUs,2GiB, a direct D:
bind and private /work and /tmp tmpfs. Exact argument arrays are retained.
Original unittest loader/runner selects only test_economy_writer_coverage_contract;
no test assertions or native recipes are changed. Collection exit0 is not all green.

| Existing check | Published source | Corrected composition |
| --- | --- | --- |
| Original writer-contract module,57 methods |55 pass,1 fail,1 error,0 skip|54 pass,2 fail,1 error,0 skip|
| Default validator |exit0|exit0|
| Release validator |exit1, executable evidence missing|same|
| Matrix --check |exit0|exit1, expected composed backup coordinates|

Actual source-stage seconds: published32.505, composition32.949.
Both ROOM insertion ambiguity and historical bandage line selectors remain the
primary-owned failures from the preceding handoff. The extra composition
provenance failure substitutes seven existing owned pinned files. Published425
selected hashes all match raw Git; composition418 match. The changed reader
header lies outside that selected set and is bound by complete source manifest,
preimage/result hashes and post-push receipt; no missing-pin invariant is invented.
The default validator remains 14 fixtures/931 routes/2,911 lexical candidate
occurrences with release_ready=false. Draft census, coverage and 791 release
blockers remain. Lexical/inventory coverage is not executable semantic coverage.
The unchanged 17 Python reader/operator modules and 16 synthetic invariant cases
are not repeated and their previous results are not relabeled as this source proof.

Changed-line formatting PASS:

```sh
wsl.exe -d Ubuntu-22.04 --exec /usr/bin/clang-format --dry-run --Werror \
  --lines=303:311 --style=file \
  /mnt/d/Dev/Tests/Duris/accounting-plan5/native-wallet-reader-20261009-d0985e9/format-input/qualify_flatfile_economic_authority.h
```

Formatter14.0.0-1ubuntu1.1 SHA-256:
`2452a487d5336d6df4b9acde979616d38a300ab602ade268c24dd8ae6088dadc`.
Pinned .clang-format SHA-256:
`662ca77c70ffb21032305e297534f208f8f09fb8f1e7901130671e22328efc93`.
The test image has no formatter; that unavailable check is retained. The initial
Ubuntu alias was absent; read-only WSL inventory identified installed Ubuntu-22.04.
No tools were installed, system files changed, Docker restarted or storage moved.
`git diff --check` passes. These checks are not compilation or predicate execution.

## Exact native regression handoff; no shared fixture patch installed

Primary owns the existing original fixture/recipes:
`tests/async/flatfile_restore_authority_fixture.cpp`,
`tests/async/test_flatfile_restore_economic_authority.py`,
`tests/async/flatfile_namespace_cases.py`,
`tests/async/flatfile_wallet_bank_cases.py`.
The local machine-readable `native-regression-handoff.json` contains this request.
No new runner, duplicate fixture, stub, relaxed warnings/flags or new API is proposed.

Use the existing native test-access writer under its original inactive epoch,
authority lock, hashes and complete bucket topology to emit genuine private
mapping/native-index metadata. Verify both mapping and native-key readers for:

- Positive native IDs1, INT32_MAX+1 and UINT64_MAX-1, exact kind1/context12/type7,
  empty name. Native ID is not the mapping authority ID or player PID.
- Refusal of ID0, UINT64_MAX, nonempty name, wrong kind/context/locator; context12
  with player locator1 and locator7 with player context0 remain distinct failures.
- Mapping and key/index modes both require a nonzero native ID; retain the existing
  bank-only zero-ID key exception, player INT32 cap, escrow UINT32 cap, claim cap,
  treasury cap, bank grammar and all original epoch/history/namespace corruption,
  journal, secure-path, lock and byte/mode/read-only inventory controls.
- Metadata alone supplies no actual native cash, revision, original successful
  birth/receipt/source history or active/birth epoch equivalence. The unchanged
  wallet-bank ledger's finish must still emit native_domain_missing and
  economic_history_missing for active unobserved accounts. Do not substitute
  the new CURRENT DTO or fabricate a record to clear those findings.

At readiness, extend and run the SAME original whole script:

```sh
python3 tests/async/test_flatfile_restore_economic_authority.py
```

Record exact fresh strict qualifier/fixture and maintained required-profile builds,
original compiler/provider list, source/engine identities, genuine observations,
positive/refusal counts and complete source/authority preservation. Fresh build
outputs belong on D: under an owned BIN_ROOT/direct Docker mount, never mixed with
historical compiled objects. Original shared source/provider dependencies stay
with primary for repair; no qualified source composition is fabricated here.

## Outstanding execution and release gates

No make/native compiler/preprocessor, predicate unit execution, native fixture,
DB/migration, maintained server, gameplay, recovery or performance test ran.
Required native regression and builds are explicitly deferred, consistent with
[the published namespace scheduling](https://github.com/Community-Duris/Duris/blob/d0985e90936b02071ae8c107ad37d8c3da58e64f/docs/persistence/economy_accounting/FLAT_NATIVE_MOBILE_WALLET_NAMESPACE_2026-10-09.md):
"Native checks remain deferred until major-plan readiness."
This explains the missing native acceptance; formatting/source metadata do not
waive it. Full original Plan5/R1–R8 remains unfinished. Prior native custody,
restore and server passes retain only their exact historical source scope.

The current migration tree and SQL prerequisite report remain unchanged: primary
must retain guarded MariaDB default representation and coherently seal schema65
runtime fingerprints/contracts before the original SQL service journeys resume.
Required durable six-source erasure/custody, full32MiB prospective overlap,
real producer/replay/restart/lost-reply observations, bounded operator/native
holdings and both-backend semantic writer qualification remain unproven. The
named solved SQL post-ACK room-coin boot issue is not a current blocker.
Inactive behavior, wallet-root ITEM_MONEY exclusions and the declined inactive
spell change remain preserved. No activation, production access/mutation,
audit autocorrection, deployment, branch switch, primary push or merge occurs.

Evidence: `D:/Dev/Tests/Duris/accounting-plan5/native-wallet-reader-20261009-d0985e9`.
Helpers: `D:/Dev/Temp/accounting-plan5-native-wallet-reader-20261009-d0985e9`.
No build output is created. Three exact source archives/manifests, preimage/inverse,
namespace providers, original module/CLI logs, formatter identities/failures,
provenance assessment, regression request and terminal container records remain.
Raw seal SHA-256: `3496fa5cb9db885dbd1ac7047f37efb56e31c02ac88d4f4c5b4d7f36cdfdc078`. Post-push delivery rehashes every sealed regular file,
binds result/remote and all owned paths and verifies all seven historical ancestors.
Curator-ready report and additive remote follow-up do not claim notebook import,
application, owner adoption or acknowledgment. Notebook remains nonblocking;
no cross-chat message is sent. The full goal remains active after this source change.

# Plan 5 published release-gate audit and shared metadata handoff — 2026-10-08

This read-only slice establishes two shared metadata defects on the refreshed
published source plus the current 27 owned overlays: 32 existing source hashes
use CRLF bytes while the published Git/Linux source uses LF, and three matrix
function locations moved after the owned backup/restore fixes. The primary owns
both shared artifacts; no registry, matrix, contract, generator or native source
is edited here. This is a concrete repair handoff and qualification record,
not a completed repair or release certification.

The default contract CLI passes, release CLI refuses and matrix check fails.
The two original test modules run 73 methods: **72 PASS, 1 FAIL, zero errors and
zero skips**. The failure is retained, not counted as an expected passing test.
Full Plan 5, R1–R8, combined accounting and activation remain unqualified.

## Branch, exact inputs and ownership

- Sole local/remote branch: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base: `4f65467b326a73982e2355b3f2db4846760cdaf5`. Result is this containing commit;
  `D:/Dev/Tests/Duris/accounting-plan5/release-gates-20261008/delivery/result.json`
  binds its full SHA, remote equality, clean state, committed bodies and ancestry.
- Refreshed and frozen primary: `7c863e1e299f5dbc9a8c42ba18c8714078bf5698`.
  Its advance from `b55c688ec1790dbd86228ac1188bc579a543d474` changes six
  documentation paths only; the private Smith/reset integration is still
  source-only, unpublished and unavailable for combined release execution.
- Tested composed tree: `6c1d1edeb3429aecc31e7760ec1fb108c78d99d1`; archive SHA-256
  `55469532659ab0c79b7c1cfe1ed100b8d81a23f42b0f307251ec25fb39e9a7b0`. Alternate-index composition overlays the 27 current
  owned files without modifying either source checkout or installing the private
  primary candidate. `source00.json` binds every path, body, mode and link.
- Published native `src` tree: `833d3085815b396861ad18a77635412212381e4b`;
  migration tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`, canonical head 64. No older 0055/0056
  result is promoted to current combined qualification.
- Before/after authentication: 6,518 regular source bodies/modes and four symlink
  targets unchanged. All 27 overlay blobs remain exact; all seven preserved
  branch tips remain ancestors. Owned committed files are this report and the
  additive `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md` entry only.

Current primary AGENTS, README, Plan 5, completion plan, requirements, review
checkpoint and newest Smith/reset handoff are retained under `primary-*` in
this evidence directory. Primary-local notebook maintenance remains nonblocking.
This report, follow-up, raw seal and post-push receipt are the additive curator
packet; notebook application, acknowledgment and primary import are unclaimed.

## Exact commands and results

The pinned image runs `python3 -u -B /evidence/observer.py`, with cwd `/work`,
network `none`, read-only root, two CPUs, 2 GiB memory, private 1 GiB source tmpfs
and 256 MiB temporary tmpfs, and a direct D: evidence mount. Image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Python 3.12.3 binary SHA-256:
`e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f`. Host metadata binding uses Python 3.12.10;
its exact path/version/hash are bound separately. No package install occurs.

| Original operation | Actual result | Elapsed seconds |
| --- | --- | ---: |
| `/usr/bin/python3 -B scripts/validate_economy_accounting.py --root /work` | Exit 0; 14 fixtures, 926 routes, 2,900 candidate sites; `release_ready=False` | 3.819228 |
| Same CLI with `--release` | Exit 1; `writer has no executable evidence` | 0.050138 |
| `/usr/bin/python3 -B scripts/generate_economy_writer_coverage.py --check` | Exit 1; `writer coverage matrix is stale; regenerate it` | 6.725524 |
| Original `test_economy_writer_coverage_contract` | 57 methods: 56 PASS, 1 FAIL, 0 errors/skips | 6.138085 |
| Original `test_audit_accounting_invariants` | 16 PASS, 0 errors/failures/skips | 0.092494 |

The observer calls `unittest.defaultTestLoader.loadTestsFromName(module)` and
`unittest.TextTestRunner(verbosity=2)` in-process for the two original modules;
this is not a claim that separate CLI unittest commands were executed. Complete
observer stage: 30.670575 seconds. `qualification.json`, three CLI stdout/stderr
pairs and the two original method logs retain the actual outputs. Container
exit 0 means collection completed; it does not turn the test/CLI failures green.

No native fixture compile, maintained build, server boot, database daemon,
migration, live snapshot, producer, player or gameplay journey runs in this
slice. MySQL, MariaDB and flatfile are assessed from existing metadata only.
The 16 invariant cases are synthetic golden fixtures, not native authority proof.
No production configuration/data is accessed. These source-audit timings are
not workload, storage-growth, checkpoint or reconciliation release budgets.

## Shared request A: bind existing source pins to published bytes

Consumer `tests/async/test_economy_writer_coverage_contract.py:145` hashes each
current source body's raw bytes and compares it with
`writers.json:candidate_worktree_evidence.source_pins[path]`. The matrix carries
the same `candidate_worktree_evidence`; its equality is also checked. The current
candidate status is `source_integrated_unqualified` and must remain so.

All 242 pinned paths have identical primary Git raw bytes and archive bytes.
210 hashes match; 32 fail. Every one of those 32 recorded hashes equals SHA-256
of the current LF body rendered with CRLF line endings. None remains different
after that representation comparison. This establishes a hash-domain mismatch,
not changed semantic content or tar transport corruption. The first original
failure is `src/economy/economic_gameplay_authority.c`: raw Git/archive SHA-256
`cc9267d56fc00abe070dba86413ea1292dba0fca736c3d2c089dd2aa7bafe9be` versus
recorded CRLF SHA-256
`fa49c04a044c576222c958ce67e5d3cf9f0bcf25f2aac97c4f21edd10dff6f0a`.
Its published `.gitattributes` explicitly requires LF. Git blob:
`4afa41bdb718068e4862f422b701fa5ad6953d56`.

Requested primary action: authenticate the intended maintained candidate first,
then update only these existing source-pin values to its published raw Git/LF
bytes, regenerate the shared matrix so its copied candidate metadata agrees,
and retain all qualification/refusal gates. If newer content is integrated,
recompute against that exact content rather than blindly copying these values.
No new schema field, wire contract or line-ending normalization in the validator
is requested. Do not remove the raw-byte provenance assertion or treat this
metadata repair as native/route qualification.

For tested primary `7c863e1e299f5dbc9a8c42ba18c8714078bf5698`, the exact LF values are:

| Existing source-pin key | Published raw Git/LF SHA-256 |
| --- | --- |
| `src/economy/economic_gameplay_authority.c` | `cc9267d56fc00abe070dba86413ea1292dba0fca736c3d2c089dd2aa7bafe9be` |
| `src/economy/item_transfer_accounting.h` | `2b855202dc496a12c2af2708deff508e28e4c40d7380ef5be187b70c0acf0198` |
| `src/economy/native_mobile_birth_recovery.c` | `f847a578a1650e3bcb273527fc720c3ba474da9e4fa4b2d6efdafee50f0bb57b` |
| `src/economy/native_mobile_birth_recovery.h` | `46263d82fe66aba4dedbe98f9637bbd64cc556718a9ff4643c37d15c4d49fcf2` |
| `src/item/item_movement_transaction.c` | `b41d72ba71a5a80e10d8538d4f272bc87013bfc00ae49f344459ff1ec7ee02a5` |
| `src/item/item_movement_transaction.h` | `f35cbc759dc15abce5b8972ca919babfd5f4c8ea41c75ec73a5c26c2473c9339` |
| `src/item/item_transfer_command.c` | `2b5c12b0c4a9437643b2e2285333b8265d871155c25f306400c5f07e568550f9` |
| `src/item/item_transfer_command.h` | `8522cbc17a29330113b685b8a4188cde4ec975d63c02e87cdeb327be4b481211` |
| `src/item/item_transfer_repository.c` | `d4e69bae26c0e512e3457482151794f247cf5e7753a2328a099df99de8bc3ffe` |
| `src/item/item_transfer_repository.h` | `d4767fb3b546b047545dcd5b821ec941c47180ef0bc2024279358dccb0b35002` |
| `src/persistence/critical_command_repository.h` | `c6cb610153dbf86aa439badcf05dea219a826567c8197cbf147407b2fd8f967d` |
| `src/economy/auction_item_claim_accounting.c` | `7fe018e60a4d16c3d188703ea70fceb48e3d7b5026e239a6017fc8930d8a572d` |
| `src/economy/auction_item_claim_accounting.h` | `73744633505c20bb315ab6eb99a3798b2a3f10693c5f8cfc4b599e57d500f83a` |
| `src/economy/auction_listing_accounting.c` | `8921edc27abdb122d2dc2f6083e65303a42b29815f32b8cb3e60c57f6c26be34` |
| `src/economy/auction_listing_accounting.h` | `6243fb0e2593ac42d58587f67f41f84230efb95350d3f0f5ffc62ded1e16dff3` |
| `src/economy/auction_native_publication.c` | `c4de84e0a4e9b52307e2320b36d86c74748bd58f9cd0234dd0d9e7941dd909ed` |
| `src/economy/auction_repository.c` | `de0dd4cd262f5cf84190b95fe53ab12d5f4252ccef642389962f66111132edf1` |
| `src/economy/auction_repository.h` | `0c4ed36ed290982b3ceff599715f77f51fd6df88bdc2b997966a47c9690ed5b9` |
| `src/economy/native_quest_coin_give.c` | `88543581facf2dca16e646a7be068539106e956142f6ec3eb473e32b5bab26f3` |
| `src/economy/native_quest_coin_give.h` | `01d1a4b20e4150e2e449266ec31da86570550c1272db3e45a3c2b199b448cff8` |
| `src/economy/native_quest_cost.c` | `036978c11d968b5dcda06e7bbc06ee7715441b4bc9c433a4b26879f16a8e2ca5` |
| `src/economy/native_quest_cost.h` | `736790a848256d26b8e1768ff4383585863c6847ff59e4852e17a3cea589f8bb` |
| `src/economy/native_quest_cost_policy.h` | `6f96696981112246fee0fd3ab1af043591c62931f87407ca3cbaf67bbdb5be15` |
| `src/persistence/economic_sql_auction_bid_transaction.c` | `2f2e402df47df7ed2d721591718eb9522c06e7c364584f7a89d0d44acae8de23` |
| `src/persistence/economic_sql_auction_money_claim_transaction.c` | `21bb5e30bb78f20168748b3ed77b3f361c34ddca786ea6d2b7694cb5c081578b` |
| `src/persistence/economic_sql_auction_retained.c` | `769a6c80d4f81edba52d6c01e4919022318a9b7fff8437ba97e1cba1d7a6e5f8` |
| `src/persistence/economic_sql_auction_retained.h` | `3eda38dc333b968f1351403654ea44231a7509447170f74ee0864b1cd48da740` |
| `src/persistence/economic_sql_auction_settlement_transaction.c` | `77d33906a2f8c67bf5585eccbb3832efd6c485c7d83f19182208bc7513385fb7` |
| `src/persistence/economic_sql_item_transfer_transaction.c` | `59fd0090bb6b85132cf2461a785c8818110f6043a7ca5fdc4d2ec441d19fee22` |
| `src/persistence/economic_sql_item_transfer_transaction.h` | `5bfdf2db0dc16bcdc835e45d8ee4b39763ecbfbe90a2e70ba5c31b8e3d5a2d77` |
| `src/persistence/economic_sql_native_mobile_birth_transaction.c` | `1587aa7e22fd9c8c89d4c556d46db71d0c80365292ec886f6b4c0f0dd6603536` |
| `src/persistence/economic_sql_native_mobile_birth_transaction.h` | `6ff60368b33f4f0f87bb6def4021b546958ade85c10eb64be1921c684f902258` |

`shared-metadata-handoff.json` retains all 242 expected/actual hashes and Git
blobs. `pin-representation-audit.json` retains both LF/CRLF digests and line counts
for all 32 mismatches. `source-provenance-binding.json` retains the first failure's
independent Git/archive check. This is a new current 242-pin finding; previously
resolved historical five-pin/153-pin handoffs are not reopened.

## Shared request B: refresh three existing source locations

The existing generator's `build()` is called without writing its shared output;
its complete result is retained as `computed-matrix.json` in evidence only.
There are no non-route differences and only these three route field differences:

| Existing route | Existing field | Checked value | Computed value |
| --- | --- | --- | --- |
| `backup.capture` | `source.definition_lines` | `[714]` | `[741]` |
| `backup.retention` | `source.definition_lines` | `[496]` | `[518]` |
| `restore.qualification` | `source.definition_lines` | `[212]` | `[214]` |

Requested primary action: after integrating the current owned backup/restore
bodies, regenerate `writer_coverage_matrix.json` with the maintained generator.
Do not hand-edit or independently import the evidence-only matrix over newer
primary metadata. Preserve all source pins, writer identities, dispositions,
backend evidence and `coverage_complete=False`/release BLOCKED semantics.
No field or interface is added. `final-assessment.json` binds the complete diff.

After both owner repairs, run the original provenance module, invariant module,
`generate_economy_writer_coverage.py --check`, default contract CLI and release
CLI against the exact combined source. The provenance/matrix checks should pass;
release must still refuse until its independently required executable coverage
exists. Current failures are not waived by those proposed repairs.

## Current coverage and completion audit

The maintained registry is `draft`; `census_complete=False`. The lexical scan
has 2,900 occurrences / 2,842 unique sites, all 2,842 mapped, zero unique additions,
removals or unmapped sites relative to its maintained census. Mapped lexical
sites do not prove semantic writer completeness, executable coverage or release.

| Existing metadata dimension | Actual values |
| --- | --- |
| 926 writer rows | 913 legacy, 6 observed, 4 enforced, 2 projection, 1 unsupported |
| Dispositions | 697 runtime mutation, 91 runtime projection, 115 nonwriter, 19 dormant, 4 offline |
| Each of MySQL/MariaDB/flatfile | 4 qualified, 1 refused, 921 unverified |
| Release-blocking routes under existing rules | 788 |
| Overlapping blocker reasons | 769 lack executable evidence; 785 lack qualified coverage; 788 lack backend qualification |
| First executable-evidence blocker | `account.item_reward` |

Backend totals include nonwriters and dormant candidates; they do not mean all
921 unverified rows are reachable unsupported gameplay writers. Blocker reasons
overlap and must not be summed as distinct routes. Existing `coverage_complete`
remains false and `playable_release_status` remains BLOCKED.
`release-assessment.json` retains every computed blocker with path, symbol,
disposition, coverage, expected backend status and evidence presence.

| Required Plan 5 / R6–R8 proof | Established scope and remaining gate |
| --- | --- |
| Semantic census and activation | Lexical mapping is current; semantic reachability, executable same-root proof, unsupported pre-mutation refusal, quiesced complete baseline and activation owner remain primary/combined gates. |
| Independent reconciler and operator views | Current original invariant module passes synthetically. Prior owned read-only/native slices retain their exact recorded scopes; this source audit adds no full native holdings, UID, provenance or player journey proof. |
| Backup/restore and retention | Prior original SQL/flat restore and eight native custody results remain bounded by their recorded inputs. The original missing-evidence native fixture still needs the shared genuine-provider composition; remote backup custody and complete retention evidence remain required. |
| Erasure and export | Existing six-source durable erasure proof handoff remains open; synthetic in-memory filtering is not the durable adapter. External-ledger and SQL generation nonempty guards stay in force. |
| Original native audit selections | Nine selections remain: stake 1, origins 3, canonical 4, child identity 1. Shared original head-62 assertions versus current 64 and provider recipes require their owner's review; known unchanged failures are not rerun here. |
| Budgets, faults and gameplay | Both backend routes, actual producer/player journeys, replay/restart/lost-reply faults, opening/cutover, workload/storage/checkpoint budgets and the unpublished combined candidate remain unqualified. |

These are release dependencies, not permission or notebook blockers for otherwise
independent Plan 5 work. Plans 1–4 retain mutation repairs and shared recipe/
contract/migration ownership. Neither the registry counts nor source checks nor
prior isolated native passes establish full completion.

## Evidence custody and delivery

Raw directory: `D:/Dev/Tests/Duris/accounting-plan5/release-gates-20261008`.
Helpers: `D:/Dev/Temp/accounting-plan5-release-gates`. No new build directory is
needed. Exact Docker command is `docker-command.json`; helper bodies, frozen
source, method logs, all metadata comparisons and final container state are
retained. Raw regular-file seal SHA-256: `7440a4937818e84549a9a1c8b185cffd43c2030bb857747ece72a563af05f5f9`.
`seal.json` binds bodies/modes without following links. Post-seal documentation
and delivery outputs are excluded from the self-writing seal and are separately
Git/receipt-bound. `delivery/result.json` rehashes every sealed body, verifies the
remote result, clean owned worktree, seven ancestral tips and all 27 overlay
blobs, and records the terminal non-OOM/network-none containers.

Only `HEAD:refs/heads/codex/accounting-plan5` is pushed. Accounting stays inactive;
wallet-root ITEM_MONEY exclusions and the declined inactive spell path remain.
No primary push, shared mutation, autocorrection, production access, deployment,
merge, cross-chat message or activation occurs.

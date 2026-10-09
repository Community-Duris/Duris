# Plan 5 maintained Smith source qualification and provenance handoff — 2026-10-08

The newly published four-file Smith save/readiness milestone changes native
inputs and establishes a new source-provenance defect: four shared pins still
bind older content. The previous 32-pin line-ending finding is historical to its
recorded source. Current results are **207 matching raw pins, 31 current CRLF
pins, and four stale-content pins**, totaling 242. All 242 archive bodies equal
published Git bytes. No shared metadata is repaired independently.

The original source checks run 73 methods: **72 PASS, one provenance FAIL, zero
errors/skips**. Default contract CLI passes; release refuses and matrix check
fails. This is source-only evidence. No compilation, native/database/gameplay
execution or release qualification is claimed. Full Plan 5/R1–R8 remains open.

## Exact source, branch and ownership

- Sole local/remote branch: `codex/accounting-plan5`.
- Owned worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `791207feca81ca88538317b0124398f31006328e`. Result is this containing commit; post-push
  `D:/Dev/Tests/Duris/accounting-plan5/maintained-smith-source-20261008/delivery/result.json`
  binds its full SHA, exact remote, clean state, committed bodies and ancestry.
- Refreshed/tested primary: `d836ea1c419a2b685d71a196ec2cf408764f4324`.
- Frozen primary plus all 27 current owned overlays: `f83fc1b4fd66e5630900227473a2efaa002447d7`.
- Archive SHA-256: `d5235ba37153920146d0d55cff4dc61b8e136e1a897e270d2f42bffc1e52edf1`.
- New published native tree: `ae5dc43f80b192d954b52bf40b7333f8dbb9ebfb`; previous
  `833d3085815b396861ad18a77635412212381e4b` is not the current native tree.
- Migration tree unchanged: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`, canonical head 64.
- Before/after authentication: 6,523 regular source bodies/modes and four symlink
  targets unchanged. All 27 overlay blobs are exact; all seven preserved tips
  remain ancestors. This report and the additive remote follow-up are the only
  owned commit changes. No shared registry/matrix/contract/recipe/native edit.

The primary advances since `ac0321d05dab64dc64a7d86eec9f69c50c6e48bd` include
four native paths: gameplay authority `.c/.h` and player save pipeline `.c/.h`.
Their aggregate source diff is 631 insertions/seven deletions. New Smith token,
queued-body retention, flat readiness and installed-wallet observation are
source milestones, not a published full Smith/room/shared-birth candidate.
The latest review retains its broader private candidate and incomplete original
transaction, producer, recovery, publication and guarded ACK dependencies.

Current AGENTS, README, Plan 5, requirements/completion/review and the two
maintained Smith milestone reports are retained as `primary-*`. Both milestones
explicitly defer compiler/native/persistence/recovery execution until major-plan
readiness. This slice respects that instruction. No prior maintained server or
native binary is adopted as proof of the new native tree; individual historical
component results keep their recorded source/dependency scopes.

## Actual checks and workload

The unchanged original observer runs `python3 -u -B /evidence/observer.py` with
cwd `/work`, network none, read-only container root, two CPUs/2 GiB memory,
1 GiB source tmpfs and 256 MiB temporary tmpfs, direct D: evidence mount. Image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Python 3.12.3 SHA-256:
`e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f`. Host Python 3.12.10 path/version/hash are separately
bound in `host-tools.json`. No new framework, package or build cache is added.

| Original check | Actual result | Seconds |
| --- | --- | ---: |
| `/usr/bin/python3 -B scripts/validate_economy_accounting.py --root /work` | Exit 0; 14 fixtures, 926 routes, 2,900 candidate sites, release_ready=False | 3.508580 |
| Same CLI with `--release` | Exit 1; writer has no executable evidence | 0.063170 |
| `/usr/bin/python3 -B scripts/generate_economy_writer_coverage.py --check` | Exit 1; matrix stale | 6.956015 |
| Original `test_economy_writer_coverage_contract` | 57 methods: 56 PASS/1 provenance FAIL; zero errors/skips | 6.355579 |
| Original `test_audit_accounting_invariants` | 16 PASS; zero errors/failures/skips | 0.145108 |

Modules execute in-process through the observer's original unittest loader and
runner. Full observer stage: 30.375285 seconds. Container exit zero means
collection finished; the original failure and CLI refusals remain failures.
`qualification.json` and raw stdout/stderr/method logs retain exact commands.
The invariant cases are synthetic golden fixtures. MySQL, MariaDB and flatfile
are assessed from existing metadata only; no database session, migration, server,
compile, preprocessing, native probe, player or actual producer executes here.
These times are not release-host workload/latency/storage/checkpoint budgets.

## Exact existing-field repair request to primary

Consumer `test_economy_writer_coverage_contract.py:145` compares raw source bytes
with `writers.json:candidate_worktree_evidence.source_pins[path]`. The shared
matrix copies that metadata and requires equality. Preserve candidate status
`source_integrated_unqualified`, coverage incomplete and release BLOCKED.

All four changed published paths have old content pinned: three hashes equal
previous Git raw bytes; gameplay authority `.c` equals previous CRLF bytes.
`stale-pin-domain-proof.json` verifies those domains. This is not explained by
rendering the *current* source with different line endings. Exact replacements
for the tested primary are:

| Existing source-pin key | Current recorded old-content SHA-256 | Current published raw Git SHA-256 |
| --- | --- | --- |
| `src/economy/economic_gameplay_authority.c` | `fa49c04a044c576222c958ce67e5d3cf9f0bcf25f2aac97c4f21edd10dff6f0a` | `616f9ca581cff1095ce7b58e33b98416935e6443f4052bb30a221f96b2f27a73` |
| `src/economy/economic_gameplay_authority.h` | `3b532d79a6960164673b91b65098d2689c7ec7ec7a0cc8d87e8a0a5a9c096e4b` | `8e20a1249b51d2c2bc214cb13753957af6db50cf9ea38c38615b5ba023389084` |
| `src/player/player_save_pipeline.c` | `77eb09369ff8bc122ccc6ff7c294ad3c5e631c2db3a14468eb04e57aed820579` | `94633125323e5cfaba935bb155205d1b994182979d49751a484710b9aa2cdb8a` |
| `src/player/player_save_pipeline.h` | `b163be37492c528bc2b704402c2dd5c1c1b2f07a8bc43713e0b24e75fd21dc2d` | `70cab34bd9e1d3f5c4af61813b9855ac69954609c367d85004724b02125bf319` |

The maintained flat milestone reports gameplay authority `.c` application hash
`a9ea76da7e39a49c69b3d03e81be6baff22320db98a269d63596d3d3c8e295f5`.
That is the **current CRLF rendering**, not its raw Git/LF hash above. The bodies
are representation-equivalent; do not label application/archive transport as a
semantic content defect, copy that CRLF hash into a raw-LF pin, normalize away
the original provenance assertion, or alter native content to satisfy metadata.

The other 31 mismatches retain exactly the previous current bodies and CRLF
explanation. Their raw-LF replacements remain in [PLAN5_RELEASE_GATE_METADATA_HANDOFF_2026-10-08.md](PLAN5_RELEASE_GATE_METADATA_HANDOFF_2026-10-08.md), excluding
its now-superseded gameplay authority `.c` row. `pin-change-audit.json` records
all 242 expected/current/raw/CRLF digests, Git blobs and changed-body flags.
Thus the complete existing-field repair is 31 representation corrections plus
these four authenticated current-content replacements. Recompute if primary
integrates newer content; preserve historical reports and qualification claims.

Primary action: authenticate intended maintained bodies, refresh those existing
pins and regenerate the shared matrix after integrating owned overlays. Only
three matrix route differences remain, all `source.definition_lines`:
`backup.capture` [714] to [741], `backup.retention` [496] to [518], and
`restore.qualification` [212] to [214]. There are no non-route matrix differences.
The computed matrix remains evidence-only and is not installed over shared work.
No new schema, field, wire format, API or mutation behavior is requested.

After repair, run the original provenance/invariant modules, matrix --check,
default contract and release CLI against the exact resulting source. Metadata
checks should pass; release must still refuse until its executable requirements
are met. Compiler/native checks remain subject to original major-plan readiness;
this source audit does not authorize or replace them.

## Release limits and evidence custody

The current lexical census remains 2,900 occurrences/2,842 unique sites, with
zero unmapped sites or unique additions/removals. This does not prove semantic
reachability or supported-writer execution. Registry remains draft with
census_complete=False. All 788 prior release blockers remain, including first
executable-evidence blocker `account.item_reward`. Per-backend metadata remains
4 qualified/1 refused/921 unverified for MySQL, MariaDB and flatfile; those totals
include nonwriters/dormant candidates and are not 921 unsupported live writers.

Nine original audit opt-ins, original shared provider/head repairs, six-source
durable erasure propagation, actual producer/player/fault/recovery journeys,
complete retention/remote custody, opening/activation owner and measured budgets
remain required. The full current native tree and private combined candidate
remain unqualified. The full objective is unchanged; this source advancement
enabled meaningful independent revalidation, not complete release acceptance.

Evidence: `D:/Dev/Tests/Duris/accounting-plan5/maintained-smith-source-20261008`.
Helpers: `D:/Dev/Temp/accounting-plan5-maintained-smith-source`. No new build output.
`source00.json`/archive bind the exact source; `source-change-assessment.json`
binds changed native paths, unchanged overlays/schema, complete matrix diff and
terminal non-OOM/network-none container. Raw regular-file seal SHA-256:
`15cc525a79ede1285b43e38cf5332226b2c30ea0aaef3d55a044e6d4ee620759`. `seal.json` follows no links. Excluded post-seal docs/delivery outputs
are separately Git/receipt-bound; `delivery/result.json` rehashes sealed bodies
and verifies committed blobs, remote equality, clean state and seven ancestors.

The report/follow-up/seal/receipt are curator-ready for the primary-local notebook;
application/import/acknowledgment remain unclaimed and notebook upkeep is
nonblocking. Only HEAD:refs/heads/codex/accounting-plan5 is pushed. Accounting
remains inactive; wallet-root ITEM_MONEY exclusions and declined inactive spell
path are preserved. No primary push, production access, autocorrection,
activation, deployment, merge or cross-chat message occurs.

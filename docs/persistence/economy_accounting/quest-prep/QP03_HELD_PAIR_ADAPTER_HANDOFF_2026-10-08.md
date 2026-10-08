# QP03 original held-pair value adapter — 2026-10-08

This bounded bundle implements the reviewed [held-pair reservation](QP03_HELD_PAIR_PROOF_RESERVATION_2026-10-08.md).
It adds an executable passive reader of original exported envelopes and narrow
captured-agreement assertions. **No genuine owner export, hold/publication/ACK,
SQL transaction, journal transition, quest-D retirement or gameplay journey was
executed by this bundle.** Successful decoding, component tests and modeled
agreement remain separate from those missing native proofs. The continuing
native Goal remains **BLOCKED**; this delivery does not resume or complete it.

## Delivery and pins

- Preserved prep parent: `7e861c68fbe70f19d574504e21c451326f955efd`.
- New code/test commit: `0ee2549cd0ddf9a8efb6315a65dd99e0fff43c69`
  (four new files only).
- This separate handoff commit follows that code commit on
  `origin/codex/accounting-quest-prep`; its exact SHA accompanies publication.
- Reviewed/tested primary candidate: `df0570c5456d4d747ca1320ce958c1db52bb08fd`.
- Publication refresh fetched `9c49043a5361d9a0f5de978bb473f8ff57b72fd9`.
  Its source, migrations, production AREA/QST and maintained fixture/build-helper
  inputs are byte-identical to the tested pin. The original candidate/binary
  labels remain unchanged; `refresh-proof.json` retains that comparison on D:.
- Candidate source tree: `833d3085815b396861ad18a77635412212381e4b`;
  migrations tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.
- Retained historical producer research: `55905eac1906cf59405764407f9d22497cccfff3`;
  original accounting base: `17c033d69316b21da8598791fc95cae79baa8dc2`.
- The reservation's primary `a9807ef2757cf79f5c5132f4753e6c936479c735`
  has these same source/migration trees. This is source compatibility, not
  evidence of additional runtime qualification.
- The preserved prep branch's own source tree is
  `3ebd0dd0ec5e12dfaa5edf83a906aea4a6ebcc7d`. Qualification used a fresh
  composition of the reviewed primary plus owned prep files on D:, not this
  older source tree. No merge/rebase, shared source replacement or earlier
  bundle rewrite was needed. Do not relabel either candidate or binary.

Owned new files:

| File | Purpose |
| --- | --- |
| `tests/async/quest_accounting_prep/read_native_quest_pair.cpp` | Passive bounded original command/context reader; actual maintained decoders and phase2 structural pair validator. |
| `tests/async/quest_accounting_prep/native_quest_pair_checks.py` | Explicit owner-observation joins for stable H1/H2 holds and terminal A/J/T attempts. |
| `tests/async/quest_accounting_prep/test_native_quest_pair_checks.py` | Modeled agreement/corruption controls, including coherent forgery's unavoidable external proof boundary. |
| `tests/async/quest_accounting_prep/test_read_native_quest_pair.py` | Maintained sanitized component controls plus modeled exports and reader acceptance/refusal cases. |
| This document | Exact delivery, commands, results and remaining owner dependencies. |

No existing helper, capture, oracle, test, driver, manifest, registry, schema,
finish plan, production source or canonical `HANDOFF.md` changed.
There is no executed SQL/flatfile backend or applied database-schema pin for
this component bundle. The migrations tree is a source pin only; unit-cut
binary/schema/lineage/epoch fields are explicitly modeled values.

## Contract and actual source ownership

The scope is original **non-fee v12 acceptance/consumption**, Auriam (16006) in
room 16077, and the successful disappearing QP03 completion. Its runtime
ingredient order is 16080, 16014, 16013, one exact selected root each; rewards
are 16075, 16015, one each. There is no fee or XP award: required economic mask 3,
XP mask 0. The reader checks the actual decoded kinds/root order, frozen branch,
literal definition/continuation link and D terms. It refuses an internally
valid alternate reward or ingredient despite exact child-command linkage.
Paid v14/fee v6 routes and other recipes are outside this contract.

Production producer/dispatch/full-boot/reset facts remain in
[SOURCE_FACTS.json](SOURCE_FACTS.json) and the reservation. This bundle uses
the original exported native birth reference and source digest, rather than
looking up a replacement recipient or constructing a birth from current stock.
The same original PID/instance/birth/source must agree across parent and child.
That comparison does not authenticate NPC birth, source custody or a reset.

The maintained [context decoder and pair validator](https://github.com/Community-Duris/Duris/blob/df0570c5456d4d747ca1320ce958c1db52bb08fd/src/world/native_quest_recovery_context.c#L774)
own structural validation. **Phase1 never calls the phase2 pair validator.**
Even a phase1 context carrying `physically_proven` remains a value carrier,
with explicit external proof requirements. Read-only files, hashes and a
structurally valid pair supply no private owner capability.

The real [save publication owner](https://github.com/Community-Duris/Duris/blob/df0570c5456d4d747ca1320ce958c1db52bb08fd/src/player/player_save_pipeline.c#L5627)
and [guarded publication ACK](https://github.com/Community-Duris/Duris/blob/df0570c5456d4d747ca1320ce958c1db52bb08fd/src/persistence/critical_command_coordinator.c#L3677)
must provide the actual physical proof/census, original command/receipt,
hold generation and confirmed hold consumption. The adapter distinguishes
that publication ACK from the original reward-obligation ACK.

The real [retirement owner](https://github.com/Community-Duris/Duris/blob/df0570c5456d4d747ca1320ce958c1db52bb08fd/src/world/quest.c#L2902)
must verify both original receipts and the exact acknowledged obligation in
the same SQL session, prove confirmed cleanup, then call guarded pair
transition. Native historical receipt verification returns `already_applied`;
all remaining receipt fields, including the full 4096-byte result array digest,
must match the original retained receipt. Economic mask 3 is an observation
from the maintained obligation reader's verified witnesses, **not an invented
column in the SELECT-only capture**. Zero XP requires no original entitlement.

The modeled joins require:

- H1/H2: same original phase1 pair, candidate/process/coordinator binding,
  exact operation/PID/command/save revision and hold generation, stable accepted
  unacknowledged SQL cut, no physical release/retired latch and no poisoning.
- A/J/T: original phase2 pair and successful SQL roots/inbox, exact retained
  receipt bytes against the capture, original literal continuation/ACK, both
  guarded publication/hold-consumption observations, original receipt and
  complete obligation verification, and same-session confirmed cleanup.
- Each actual terminal attempt: strictly increasing sequence, no successor,
  full attempted postimage size/digest and explicit complete-image observation,
  guarded coordinator return, retained contexts and uncertainty disposition.
  An uncertain retry keeps the exact pair, generation and whole postimage.
- Successful return sets retired and removes both actual contexts. A latched
  repeat carries verification from that successful sequence, has no new pair
  attempt and no journal result. Absence, journal OK or an ACK row alone fails.
- Live handoff1 requires its actual successor; the maintained non-live
  handoff1 terminal cleanup exception is preserved. An already moved reward
  remains untouched; cleanup must preserve all captured projections/affects.

Public APIs are `assert_qp03_held_pair(before, after, pair_before, pair_after,
owner_before, owner_after)` and `assert_qp03_pair_retirement(before, after,
pair, attempts)`. The new tests provide explicit, labeled modeled input shapes.
These are same-process agreement assertions, not a new cold-recovery owner.
Every reader/assertion pass returns external requirements for authentic export
and candidate/binary binding, real guarded owner execution, D authority,
physical custody/full census/chronology and original hold/SQL/ACK/journal
observations. A coherently forged report can satisfy comparisons and still
cannot self-authenticate; that limit has an executable counterexample.

## Reader and isolated commands

Input is two original canonical command bodies and exact context attachments,
plus owner-observed revisions/phases. No journal directory, copied journal,
journal initialization/scanner or SQL/world/owner API is used. The reader
requires nonempty read-only regular files, refuses final symlinks, checks
stable metadata plus two complete identical reads/EOF, and enforces 512 KiB
command/32 MiB attachment bounds. Maintained canonical decoders reject trailing,
truncated or invalid values. Validation refusals produce no stdout and one
fixed public stderr line. Output is under 64 KiB and contains digests/identities/
progress only, without private names, definition strings, messages or inventories.
Read-only mode is an input guard; authentic immutable export remains external.

After building in a compatible isolated primary composition:

```sh
"$READER" parent.command parent.attachment "$P_REV" 2 \
  child.command child.attachment "$C_REV" "$C_PHASE" > pair.json
```

Use the actual immutable owner export and observed revisions, never modeled
fixture bytes or values recovered from current images. `READER` is the actual
`reader_path` in the retained component receipt; record its SHA alongside any
later genuine use. No genuine owner-export invocation is claimed here.

Actual qualification root:
`D:\Dev\Temp\qp03-held-pair-adapter-20261008\candidate`.
Run inside Ubuntu-22.04 with `TMPDIR=/mnt/d/Dev/Temp`:

```sh
python3 tests/async/quest_accounting_prep/test_native_quest_pair_checks.py
python3 tests/async/quest_accounting_prep/test_read_native_quest_pair.py
python3 tests/async/quest_accounting_prep/test_quest_cut_checks.py
python3 tests/async/quest_accounting_prep/test_native_quest_retirement_checks.py
python3 scripts/zone_story_quest_catalog.py --source-root . \
  --production-output ../production-catalog.json --check
```

Formatting in the prep worktree used the maintained
`./scripts/format.sh --file tests/async/quest_accounting_prep/read_native_quest_pair.cpp`
and its `--check` form. Staged whitespace and Python syntax checks also pass.

The native wrapper reads the maintained fixture runner's source/flag/link/
deadline values. It retains C++20, O1/debug, warnings-as-errors, ASan+UBSan,
frame pointers, non-PIE, pthread and section controls, with the 20-second native
runtime deadline and maintained 600-second per-compiler-command timeout.
`native_build_artifacts.build_native` keeps its actual input fingerprints,
immutable output/cache verification and bin-root guards. Both binary/cache
paths are beneath the D: candidate's `bin/`.

All 17 provider translation units are real maintained sources. The three
additional current command-codec providers are `native_quest_cost.c`,
`native_quest_coin_give.c` and `lockpick_retirement_continuation.c`; no owner
stub resolves linkage. The generated fixture retains the complete maintained
context-test body, changing only `int main()` to `void maintained_controls()`
and appending a modeled export main. Its genuine allocator observer/wrapper
still delegates to the actual allocator. Only that fixture uses the original
allocator link wrapper; the passive reader has no corresponding wrapper.
The empty FIFO refusal control uses `/dev/shm` because DrvFS cannot create
FIFOs; it is removed after the control. All other artifacts/evidence remain on D:.

## Results and import boundary

The source check compared all 1,607 selected source/catalog/provider blobs
against the primary Git objects. Proof:
`D:\Dev\Temp\qp03-held-pair-adapter-20261008\source-proof.json`, SHA256
`c8e8f2e6d4105e79f7d66bef456ebda3452ba182f202a7aa71f692d1fb5250d4`;
the exact local check is retained as `verify_source.py` beside it.
The production catalog validates with 2,668 definitions from `areas/AREA`.
It excludes dynamic bartender quests and supplies no native runtime proof.

| Executed check | Result and scope |
| --- | --- |
| New Python agreement suite | **14 tests PASS**, modeled inputs only; `python-agreement-delivery.log`. |
| Actual sanitized native component/reader | **33 reader invocations PASS**: 3 accepted original-value cuts, 30 strong refusals, plus the complete maintained context-test controls; `qualification-delivery.log`. |
| Unchanged cut oracle suite | **17 tests PASS**, modeled/component scope retained; `unchanged-cut-controls.log`. |
| Unchanged temporal retirement suite | **13 tests PASS**, modeled agreement scope retained; `unchanged-retirement-controls.log`. |
| Owned bytes | All four committed files exactly match the D: candidate bytes, including Git blob identity; `owned-input-proof.json`. |
| Formatting/syntax/whitespace | Maintained C++ formatting/check, Python syntax and staged whitespace checks PASS. |

Final component directory, relative to the qualification root:
`bin/tests/qp03-pair-reader-q7d7jjrb`.
Its `receipt.json` is **`passed_component_only`**, with `source_unchanged:true`,
`owner_execution:false`, `SQL_execution:false`, `journal_access:false`.
Its full provider/source ledger, compiler/sanitizer controls, runtime deadline,
actual binary paths, immutable modeled-export digests and all 33 results remain
retained there. These are component ELF binaries, not a game/server ELF:

- Reader SHA256: `cbaa254974ac059059edeae5bdba1da8052f2b11abe13af2841f16748d517d77`.
- Fixture ELF SHA256: `dd7f118d3819fbc6dfb9808ea43ecd8b2fc36dbe26c7813d757c884ad71cdfaf`.
- Component receipt SHA256: `8f78c6b1ef5e52e81cc717fe7529ccad20bf4ed80fe33b71ab4af96bf03f6e0d`.
- Compiler: `g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0`.
- Owned-input proof SHA256: `96a2b35962e6f502c6e14f58523cee13b6fd5fa9c1ded7ff5902ca2fe34dee0a`.
- Refreshed-primary comparison SHA256: `91d2c5b2844f04581b1c3a8eeb7cde8a38a130d059bf01ebf5ffab4e410930b3`.

Evidence/log filenames without a prefix are beneath
`D:\Dev\Temp\qp03-held-pair-adapter-20261008`. The final component receipt
is beneath its `candidate/bin/tests/qp03-pair-reader-q7d7jjrb` subdirectory.

Retained initial failures remain visible:

- `candidate/bin/tests/qp03-pair-reader-o1z650a0`: real provider links passed,
  then the modeled export fixture incorrectly reused `quest_action` from a
  no-reward fixture. The maintained successful non-fee constructor correctly
  refused it. The fixture now supplies modeled `quest_completion`; no guard
  or production code changed.
- `candidate/bin/tests/qp03-pair-reader-254qdfci`: export/reader controls ran
  before `mkfifo` failed with DrvFS `Errno 95`. The final fixture executes the
  original FIFO refusal through an empty Linux tmpfs FIFO; no control is skipped.
- `candidate/bin/tests/qp03-pair-reader-aupud8ng`: intermediate 33-invocation
  component pass. It predates the explicit external-proof list in the reader
  JSON and is superseded by the final binary/receipt above.

No earlier failed/intermediate binary is substituted for the published code.

Cherry-pick the code commit into the reviewed primary source or a compatible
descendant, then this handoff commit. Preserve consumed earlier prep bundles.
Re-run the focused commands after a material source/provider change; do not
merge the prep branch's older production tree over the primary candidate.

Remaining shared prerequisites are unchanged: authentic original envelope and
H/A/J/T exports from existing guarded owners, exact SQL receipt/obligation/
session-cleanup execution, whole attempted postimage and guarded terminal
return/latch observations, and actual quest-D whole-stock/cash retirement with
full literal world/custody census and chronology. The current successful-D
publication refusal still requires its real production owner. This bundle
does not implement that owner or create activation/birth/source authority.
Keep genuine journey/recovery and final qualification on the integrated
primary candidate at its agreed major-batch boundary.

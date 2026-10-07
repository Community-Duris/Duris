# QP06 flat later-drop prerequisite diagnosis — 2026-10-07

The original Kord XP-ACK/recovery/later-drop journey remains **RED**. The owned
read-only diagnostic now identifies the exact refusal and its earlier durable
room history. It does not repair or qualify shared movement/recovery authority.

## Pins and executed result

Accounting source f04317d9d72aa5594448809baad6041936b09801, later published accounting
a7b3181edb80bf188f616c39b8e7cb144e11cb18 and d91f59af06239a5736d10498b89091699c5e05c6
have identical qualification inputs. Worker ELF source is the actual build
6db65f624836f150ed3dfe33508e1f1719145fdb, including QP02/QP07 fixes. Do not relabel
the binary as a documentation refresh. Research PR678 remains
55905eac1906cf59405764407f9d22497cccfff3.

| Input/evidence | SHA256 |
|---|---|
| Actual flat ELF | 8aedc856f2ff9cfee15ce694b16d28540daba1a5f41dedd40d59de91ad1b1ea5 |
| Flat provider source | 26c87c0989a623b486d77edf02eb31ec0de9a2ba4ed19fb10722433d77fc9ddd |
| Movement admission source | 18dfd4dbd07f44674e2feb8ff633b82547bf1f3e796f56da0ab7f98aaf7d5ef1 |
| Executed owned observer | 07fe2399e74975361d110a7845a313a6a453276d88f192c12ac928a182b9286c |
| Maintained schema manifest | 1fddd009cc67bba940efba28c2afc3d16d65ea8a476410034cd77c90a0a9aea2 |
| Private final diagnostic JSON | d7a77d3cbd6ee6834b5ab7bbe957f08a408a19b72278d587a9ea16ea4a7488b7 |

Final focused run: exit0, **87.885 seconds**, diagnostic PASS/original journey RED.
Docker desktop-linux, owned container quest-prep-runtime-20261007, QA image
sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b.
Network disabled; MariaDB not started; no production environment read or changed.

## Exact observed prerequisite and revision chain

At `src/flatfile/flatfile_item_repository.c:879`, actual apply_transfer compared:

| Predicate | Expected | Durable observed | Disposition |
|---|---:|---:|---|
| Player owner (type1, id1, context0) revision | 8 | 8 | Matches |
| Room owner (type3, id22800, context0) revision | 0 | 7 | **Fails** |
| Reward identity/kind | UID821, VNUM29237 | UID821, VNUM29237 | Matches |
| Reward root/parent | 821 / 0 | 821 / 0 | Matches |
| Reward item revision/state | 1 / active1 | 1 / active1 | Matches |

The provider returns ESTALE116 before any item effect. The matching item fields
were read from the catalog; later item predicates were not executed. The stack
was apply_transfer → flatfile_item_repository_apply →
flatfile_critical_command_repository_apply_selected → flatfile_accounting_apply_selected
→ worker_main. No assertion/result, item UID, counter, journal or authority was changed.

The added non-stopping observer in the original fault GDB recorded every earlier
transfer touching the actual fixture room. Successive provider revisions and
three SELECT-equivalent native inspector cuts establish this chain:

| Genuine command/provider action | Room revision before → after | Evidence |
|---|---|---|
| Initial `drop all`, reason player_drop6 | 0 → 1 | One batch of19 genuine starter roots; next provider sees room1 |
| Adoption of secret ear29262, reason creation2 | 1 → 2 | System→room; next get sees room2 |
| `get ear`, reason player_get5 | 2 → 3 | Room→player; next adoption sees room3 |
| Adoption of secret scalp29263 | 3 → 4 | Next get sees room4 |
| `get scalp` | 4 → 5 | Next adoption sees room5 |
| Adoption of secret toe29264 | 5 → 6 | Next get sees room6 |
| `get toe` | 6 → 7 | Before-offering durable inspector sees room7 |
| Offering/reward XP-ACK crash and recovery | 7 → 7 | At-crash and recovered cuts both room7,19 starter roots |
| Later `drop dagger` | 7 remains7 | Admission expected0; provider refuses before effects |

The room owner was first created/advanced by the successful initial starter batch,
then advanced six times during authentic search/get of the three secret fixture
ingredients. This is not a reward-created room counter or stale player revision.
Player owner revisions were6 before offering,8 at crash and8 after recovery.
The previously qualified reward remains in player custody after refusal; no room
materialization, later-move success or second cold move qualification follows.

## Actual admission and cold-boot source route

The recovered command's actual admission breakpoint recorded
`live_drop_token=false`, `accounting_active=false`. This is the ordinary legacy
flat fixture path, not active native SQL drop. `src/cmd/actobj.c:919` routes ordinary
submit_player_drop to item_movement_transaction_submit; the active SQL branch is
guarded and excluded in this maintained flat build. The single-root admission at
`src/item/item_movement_transaction.c:2548` reads both revisions with
item_ownership_runtime_owner_revision. Do not confuse it with submit_batch's
similar reads at3012, or the live_drop_token branch at2540: that branch uses peek,
requires a cached player revision and explicitly keeps an optimistic zero for an
uncached room without installing a cache entry. It was not exercised here.

`src/item/item_ownership_runtime.c:554` returns a cached revision; for a valid
uncached owner it installs and returns0. The payload uses that result. On cold
boot, `src/world/db.c:797` calls restoreCorpses only outside mini mode and copyover;
the actual recovery log confirms the mini-mode skip at849. Flat restoreCorpses
(`src/core/files.c:4521`) calls flatfile_corpse_restore_catalog. That shared room
catalog restoration enumerates room records, authenticates durable ownership and
materializes/hydrates room revisions. `materialize_room` at256–289 includes an
explicit empty-item branch that hydrates the durable owner revision even when
there are no live item roots. restoreSavedItems in flat mode assumes this earlier
room-catalog restoration has already happened.

This source route plus the actual0-versus7 payload identifies a missing cold
fixture room-cache restoration prerequisite. The native run contained19 durable
room roots, so the empty-room variant is **source-verified, unexecuted**. A full
world cold boot was not executed by this diagnostic; it is not evidence of a
general full-boot defect or an active SQL/live_drop_token defect.

## Smallest applicable executable command and retained evidence

Run in the existing isolated source tree/container `/current`, using the retained
ELF and a new evidence directory (the tool refuses overwrite):

```bash
python3 -B tests/async/quest_accounting_prep/diagnose_flat_reward_drop.py \
  --server /evidence/current-flat/server/dms_new \
  --server-sha256 8aedc856f2ff9cfee15ce694b16d28540daba1a5f41dedd40d59de91ad1b1ea5 \
  --source-commit 6db65f624836f150ed3dfe33508e1f1719145fdb \
  --evidence-dir /evidence/followup-flat-drop-chain
```

Executed via `docker --context desktop-linux exec quest-prep-runtime-20261007
bash -lc 'cd /current && <command above> > /evidence/followup-flat-drop-chain.out 2>&1'`.
It calls the maintained driver with `expect_recovered=True`, `fault_phase="xp-ack"`,
`quest_case="QP06"`, `move_reward=True`. The original XP-ACK fault breakpoint,
kill, fee/XP/custody assertions, boot deadlines and20-second drop deadline remain.
The added initial observer returns False at every breakpoint; the recovered
observer reads memory and detaches before the provider proceeds. GDB calls no
inferior function and writes no inferior variable. Timing is diagnostic only.

Private evidence root on host:
`C:\Users\alexa\.codex\worktrees\accounting-quest-prep\NewDuris Max\bin\tests\quest-implementation-20261007`.
Final subdirectory followup-flat-drop-chain retains diagnostic.json, both GDB
scripts, provider-observation.log, original run/logs and state. Earlier
followup-flat-drop-observer retains the initial std::array reader failure;
followup-flat-drop-observer-array retains the corrected guard-only observation
(exit0,87.675s). These are distinct runs, not hidden failures or repeated batches.
The richer final run closes the admission/earlier-room-chain questions. Raw
state, logs, account/player files and binaries remain ignored and uncommitted.

## Precise shared-owner request

Primary/shared boot and movement owners should select the supported way for the
maintained isolated mini cold-boot fixture to restore existing durable room
ownership, including an existing empty room. Review whether the original fixture
needs the real shared restoration hook or a supported boot mode. Preserve exact
durable room counters and strict ESTALE; do not reset room7 to0, invent source
authority, weaken the predicate or alter the original drop expectation/deadline.
After the owner change, rerun the original QP06 flat XP-ACK/later-drop journey on
the integrated candidate and qualify exact UID movement plus second cold custody.
Separately qualify active native SQL admission/birth/retirement at its existing
batch boundary. This pack supplies diagnosis and reproducibility, not that fix.

# Static quest story history and recovery boundary — 2026-10-08

Static story credit retains a stable original tracking key and frozen completion
context, but its runtime constructs the story transaction using the **current**
season and current catalog definition revision. Deduplication compares the full
serialized transaction, not just that key. A retained old transaction can therefore
conflict with a retry reconstructed after a season/revision change. A removed
definition can refuse even earlier. This is current source behavior, not a new
original-season policy or an executed native defect finding.

The story authority is a whole-service document, separate from player saves,
dynamic `world_quest_accomplished`, item/coin/XP effects and original obligation
ACK/pair retirement. Memory restoration after a reported story save error does
not establish that the authority stayed unchanged. These distinctions are the
additional recovery boundary beyond the accepted completion summary.

## Delivery and source pins

| Input | Exact revision / scope |
| --- | --- |
| Preserved prep parent | `1c90a27fabfcf7b2cfbdf4bc0459996a175aea28` |
| Fetched public accounting | `0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a` |
| Compared preceding public pin | `07c0e0398e0123ad9232e162cd1d1df08fd38a6d`; unchanged source/migration/test trees |
| Public source / migration / test trees | `833d3085815b396861ad18a77635412212381e4b` / `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` / `a9559094ab4b9d184de97cb0ac135ed146467b73` |
| Delivery commit | Containing commit: `git log -1 --format=%H -- docs/persistence/economy_accounting/quest-prep/QUEST_STORY_HISTORY_RECOVERY_BOUNDARY_2026-10-08.md` |

Only this document changes. No production/helper/test/capture/decoder, schema,
registry, shared driver, canonical HANDOFF, finish plan or Plan 5 file changes.
The actual continuing native Goal remains **BLOCKED**. Public changes supply no
new native quest API; private source and native execution exports remain unavailable.
Forty-two exact public bodies are exported with Git blobs/SHA256s under
`D:\Dev\Temp\quest-story-history-boundary-20261008\public`; adjacent manifest,
anchor/link/design proof and publication receipt retain this delivery's checks.

Reuse [reward-completion boundary](QUEST_REWARD_COMPLETION_AUTHORITY_BOUNDARY_2026-10-08.md)
for original economic effect/save/ACK/pair ownership and
[group-XP handoff](QUEST_REWARD_XP_ASSERTION_HANDOFF_2026-10-08.md) for its closed
captured-agreement scope. [CURRENT_CAPTURE_EXECUTION.md](CURRENT_CAPTURE_EXECUTION.md)
retains the genuine legacy Kord pins/results. Original PR #678 producer research
`55905eac1906cf59405764407f9d22497cccfff3` and existing case specs retain their
original scope. None is rerun or promoted to native/history recovery here.

## Required call chain and identity

| Stage | Existing owner and source-supported behavior | Recovery implication |
| --- | --- | --- |
| Boot and catalog | `boot_db` calls `boot_zone_story_quest_state` after special assignment; no-specials mode explicitly loads quests before runtime bootstrap. Runtime boot builds/publishes production catalog, constructs a new feature service, loads authority and enables readiness only on success. [db.c133](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/db.c#L133), [runtime.c151](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_runtime.c#L151) | Missing state is accepted as empty. Invalid/version-stale or failed load disables story service; it does not automatically migrate, purge or certify old history. This is a story boot fence, not native world activation. |
| Production definition identity | `canonical_completion_key` sorts numeric give and receive `(kind,number)` entries, retains duplicates and disappearance, and encodes them with giver VNUM into `zone-story:qst:<giver>:<hex-key>`. Identical contracts deduplicate; every Q pointer gets a binding. Message/text, traversal order, birth/reset instance and source filename are not the key. Zone derives from giver VNUM. Production content revision is explicitly1. [production.c163](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_production.c#L163), [239](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_production.c#L239), [production.h13](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_production.h#L13) | Stable story definition identity does not authenticate which native NPC/source accepted original inputs. Content revision is not an automatic hash of every live area change. Sorting for story identity does not change quest admission/recipe order. |
| Original completion input | Frozen offering continuation stores definition ID, zone, direct/credited PIDs, room, completion time, actor name/level/racewar and party context. It has no original story season or story content-revision field. `recover_pending` forwards these original fields to `record_authoritative_completion`. Native runtime/birth/custody authority remains outside the story transaction. [continuation.h32](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/item/quest_reward_continuation.h#L32), [quest.c1344](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/quest.c#L1344) | Do not derive an authoritative original season from the timestamp or invent a frozen revision. The old version1 fallback rematches exact rewards and uses the legacy completion hook; modern frozen context is distinct from that fallback. |
| Retained tracking key | Version6 uses `native-fee-action-v6-<original offering op>`; other durable offering versions use `legacy-offering-v1-<first original consumed root>`. Ordinary legacy completion without a nonzero offering UID instead falls back to runtime-generated season/PID/definition/time/sequence key. [quest.c1324](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/quest.c#L1324), [940](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/quest.c#L940), [runtime.c94](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_runtime.c#L94) | Preserve actual original op/root identity. A freshly generated key is not a retry of the retained key. The sequence generator is process-local, not a persisted completion identity. A version5 key's legacy spelling does not classify its caller as a legacy native journey. |
| Current reconstruction | Runtime requires ready service and current matching definition, then assigns current season and definition content revision. Season preference is valid SQL epoch, configured `ZONE_STORY_SEASON_ID`, then1. Captured time and recipient order are forwarded. [runtime.c137](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_runtime.c#L137), [252](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_runtime.c#L252) | Current catalog admission can fail before deduplication. If original key exists but reconstructed serialized fields differ, replay becomes conflict. If no retained fact exists, current season can receive the first accepted fact; this document does not change that policy. |
| Feature apply and deduplication | `apply_transaction` validates serialization and current definition active/eligible/zone/revision before existing-key comparison, unless loading with `allow_stale`. Full transaction serialization includes key, definition, zone, direct PID, room, time, season, revision and **ordered** credited PIDs. Exact bytes return already_applied; same key/different bytes returns conflict. [feature.c478](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_feature.c#L478), [529](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_feature.c#L529), [tracking.c215](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_tracking.c#L215) | Equal recipient sets in a different order are not the same serialized fact. Definition removal/ineligibility can reject an otherwise byte-identical retained transaction before already_applied. Neither response proves economic completion. |
| Credit, telemetry and daily effects | New transaction creates per-season/PID credit state for nondeleted recipients. Daily policy defaults off; enabled assignments/reward keys use existing time/definition/revision/recipient rules. Completion telemetry defaults observed/success with duration0 and uses `completion:<tracking key>`. Telemetry conflict attempts prior-state restore. [feature.c574](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_feature.c#L574), [feature.h46](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_feature.h#L46) | Already_applied is a domain result, not a persistence ACK; it can still revisit daily/telemetry logic and runtime still saves. Do not invent measured quest duration or new daily reward rules from this hook. |
| Save and return to offering owner | Runtime accepts applied/already_applied, serializes and writes the whole service state, then returns true only on provider success. On save failure it calls `deserialize_state(before)` and returns false. Quest recovery uses tracking success in its ACK eligibility while economic successors retain their separate work. [runtime.c297](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_runtime.c#L297), [quest.c1391](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/quest.c#L1391) | A tracking failure can leave the original obligation unACKed despite completed economic effects. Restoring memory does not undo a visible/durable provider write. The saved story document is not an exact player XP save receipt or original native checkpoint ACK. |

## Persistence, restore and content changes

`save_persisted_state` writes the complete serialized service, not a selected
player row. SQL uses singleton `state_id=1`, state_version1, catalog revision and
MEDIUMTEXT blob; normal save is an unconditional UPSERT with no per-player CAS.
Load requires exact expected catalog revision and schema. The save function
opens or resolves no SQL transaction: its ok result reports the
statement boundary, not an original-operation commit receipt. The actual ambient
transaction/commit owner remains material, especially for erasure.
Its 16 MiB input guard is **not** the complete SQL formatting limit: its call to
`qry_at` must fit the full escaped INSERT into `MAX_STRING_LENGTH=65536`, or the
formatter refuses before execution. Existing fake-SQL erasure coverage does not
exercise that real formatter. [runtime.c67](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_runtime.c#L67),
[SQL repository.c96](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/sql/zone_story_quest_state_repository.c#L96),
[qry_at3818](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/sql/sql.c#L3818),
[config.h141](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/core/config.h#L141).

Flat state has a versioned, catalog-revision-bound, length/SHA256-checked envelope
under `domains/zone-story-quests.state`, bounded at64 MiB including its header.
Load/save acquire real authority locks and recover prior authority transactions;
save also holds the story lock. It calls `flatfile_atomic_write`, whose underlying
writer renames before directory fsync. A directory-sync error returns false
**after publication**; this wrapper does not expose the writer's published flag.
Thus a valid newly visible file and a runtime false return can coexist. Whether
that file survives a real crash needs actual recovery evidence, not a successful
memory restore or checksum alone. [flat state.c161](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/flatfile/flatfile_zone_story_quest_state.c#L161),
[181](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/flatfile/flatfile_zone_story_quest_state.c#L181),
[store.c85](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/flatfile/flatfile_store.c#L85),
[161](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/flatfile/flatfile_store.c#L161).

Feature serialization retains global transaction facts plus per-season/PID credit,
names, daily assignments/reward keys, deletion/exclusion markers and telemetry.
Restore parses records, replaces its maps, then reapplies retained transactions
with `allow_stale=true` and daily awarding disabled. Old facts can therefore load
without current catalog eligibility, while new/retry completions still require
it. Current progress/summary filters current eligible definitions. Retained facts
and current displayed totals are different observations. [feature.c1376](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_feature.c#L1376),
[1445](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_feature.c#L1445),
[1607](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_feature.c#L1607),
[summary809](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_feature.c#L809).

An outer catalog-revision bump refuses an old SQL row/flat envelope at load;
there is no automatic migration in this chain. A contract removal/change under
the same outer revision can instead load old facts but remove the original
definition needed by retry. A changed season or otherwise admissible changed
definition revision alters serialized transaction bytes and conflicts with an
existing original key. These are separate cuts; do not claim a revision-stale
boot reached live retry. Current production revision1 is unchanged by this work.
Actor name/level and party telemetry are outside serialized transaction bytes;
their separate observation can still conflict after transaction deduplication.

Runtime's save-failure restoration is an **attempt**, not a strong rollback proof:
it does not check the returned restore boolean, and the captured `before` document
is not explicitly checked for emptiness before mutation. Deserialize can fail
during transaction application after replacing maps. Bootstrap/refresh failure
does disable readiness. Existing serialization-failure tests establish refusal
to return a partial serialization; they do not prove every caller restoration
succeeds. SQL's ordinary save also returns an error enum rather than a joined
original-operation receipt/readback. Do not infer committed or rolled back from
that enum alone. [runtime.c297](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_runtime.c#L297),
[feature.c1567](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_feature.c#L1567).

## Erasure is not an immutable accounting journal

`erase_character_all_seasons` removes the selected PID's character/exclusion
state, marks current/observed seasons deleted, removes every **global group
transaction containing that PID**, and removes that PID's telemetry. Other
recipients' aggregate credit masks can remain and survive reload without that
global transaction. The maintained feature harness explicitly verifies retained
peer completion after deletion. Serialization suppresses transactions involving
deleted PIDs. `remember_character` can clear a tombstone for the specified season;
recording rejects a currently tombstoned direct completer. This is the existing
erasure/re-identification policy, not an invitation to recreate a native actor.
[feature.c699](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_feature.c#L699),
[641](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_feature.c#L641).

SQL account deletion locks the singleton in its caller-owned transaction,
erases aliases and saves the aggregate; it does not independently COMMIT.
`sql_delete_account` owns commit/rollback, then successful account deletion
refreshes runtime cache from authority in the compiled non-`_PFILE_` cleanup
path, disabling publication on refresh failure.
The ordinary character cleanup hook instead calls runtime erasure and can return
reconciliation_required after durable character cleanup. Flat character deletion
prepares a domain replacement inside the existing authority bundle; its story
preparer uses season1 plus observed seasons, not an invented current-season
argument. Its caller commits the bundle. These flows must retain their actual
owner ordering; do not splice per-PID memory edits into a singleton replacement.
[SQL erasure145](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/sql/zone_story_quest_state_repository.c#L145),
[sql_player6063](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/sql/sql_player.c#L6063),
[account222](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/account/account.c#L222),
[runtime191](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/world/zone_story_quest_runtime.c#L191),
[files2233](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/core/files.c#L2233),
[flat preparer213](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/flatfile/flatfile_zone_story_quest_state.c#L213),
[flat commit405](https://github.com/Community-Duris/Duris/blob/0e13d0fb49e1ff6a31b9c06d6dfec414fb629d3a/src/flatfile/flatfile_character_delete.c#L405).

Story fact deletion is therefore not proof that an original quest was never
completed, or that an obligation is ACKed/unACKed, or that financial evidence can
be erased. Retained peer aggregate credit alone cannot reconstruct the deleted
original fact. Original economic/XP/ACK/parent-child evidence remains with its
existing owners and Plan 5; this document proposes no erasure or reward rewrite.

## Existing coverage to reuse, not rerun

These are source-inspected test scopes, **not newly passing results**:

| Existing command under `tests/async/` | Concrete existing coverage / limit |
| --- | --- |
| `python3 -B test_zone_story_quest_tracking_contract.py` | Typed valid transactions, credited/direct PID constraints, deterministic serialization, catalog validation/eligible counts. |
| `python3 -B test_zone_story_quest_repository.py` | Identical replay already_applied, same-key changed room conflict and PID/season/zone/revision filters in the in-memory repository. |
| `python3 -B test_zone_story_quest_feature.py` | Actual feature replay, telemetry-conflict restoration, daily/credit/serialization round trip, all-season erasure/tombstones and preserved peer aggregate credit. |
| `python3 -B test_zone_story_quest_serialization.py` | Allocation-failure serializer controls reject partial documents. No actual runtime save/restore/season reentry. |
| `python3 -B test_zone_story_quest_production.py` | Real booted-index catalog construction, duplicate Q identity/pointer binding and metadata. No actual runtime persistence wrapper. |
| `python3 -B test_zone_story_boot_without_specials.py` | Extracted actual boot-owner body, normal/no-specials/failing-bootstrap controls with a substituted runtime bootstrap. No full native boot. |
| `python3 -B test_flatfile_zone_story_quest_state.py` | Actual file round trip, stale revision/checksum refusal, erasure preparation unchanged-before-bundle, commit/retry and retained other PID. No post-rename directory-sync runtime failure. |
| `python3 -B test_sql_zone_story_quest_erasure.py` | Actual SQL repository against fake SQL boundary: caller transaction, locked read, revision/null/malformed/write/allocation refusals and alias removal. No database service, real `qry_at` formatting or runtime season reconstruction. |

Harness bodies/wrappers are pinned in the D: manifest. The distinct missing slice
is the **runtime's** reconstruction and restore/reload around a concrete provider
publication error, not generic transaction conflict or another erasure test.

## One bounded future reservation

Reserve one optional **runtime story reentry after post-publication save error**
component. Proposed future owned paths, not added by this delivery:

- `tests/async/quest_accounting_prep/story_history_runtime_reentry.cpp`
- `tests/async/quest_accounting_prep/test_story_history_runtime_reentry.py`

Actual available inputs/providers: real `zone_story_quest_runtime.c`, production
catalog and feature/tracking/catalog sources; maintained flat state/store/authority
transaction sources; existing production harness's minimal booted Q/index globals;
public persistence-mode/root and season interfaces; public runtime `bootstrap`,
`record_authoritative_completion` and service serialization. Reuse maintained
strict C++20/warning flags and isolated D: temporary layout. SQL symbols needed
only for linking may be explicit test stubs; they must not imply SQL execution.
The flat component's `sql_season_epoch` stub returns0, allowing the actual runtime
environment-season fallback; mode/root select only its disposable flat directory.
No replacement tracker, parser, reward policy or original-season authority.

Manual expectations and schedule for that **single component family**:

1. Use a minimal valid production Q/index fixture and disposable flat root; daily
   policy disabled. Record one explicit original key, definition, zone, PID/set,
   room, positive time and original actor metadata under configured season S1.
   Healthy record/replay is a control: true return, one identical retained T fact.
2. On a fresh isolated root, arm a one-shot error at the actual store's directory
   fsync **after successful rename** during the original runtime save. Require
   proof that the injected call is that boundary, not earlier lock/recovery I/O.
   Runtime returns false; with healthy pre-state serialization, memory restores
   the prior document, while the new valid file is visible. Do not claim its
   survival through power loss from this same-process readback.
3. Disable injection and reload through actual runtime bootstrap/flat load. The
   retained T fact still encodes S1. Configure S2 and call the same original key
   and original captured fields again. Current reconstruction changes season;
   existing full-byte comparison conflicts, runtime returns false, no second
   fact is inserted and the retained authority remains unchanged. Expected
   result preserves current source behavior, not a proposed policy correction.

The future runner command would be
`TMPDIR=/mnt/d/Dev/Temp python3 -B tests/async/quest_accounting_prep/test_story_history_runtime_reentry.py`;
it does not exist yet and was **not executed**. Bound future compilation/execution
with existing component timeout patterns. Runtime APIs and real filesystem cut
exist, so this component reservation does not require invented owner exports.
Its result would remain component behavior, not an original native journey or
cold-process recovery. No second future acceptance slice is reserved.

Actual native execution still needs authenticated original participant/current
world, original continuation and story key, provider outcome/readback, guarded
save/checkpoint/obligation ACK timeline and real cold/replay observations. The
existing quest capture selects dynamic history, not this story singleton; a
per-player cut is not owner-wide story authority. Those exports/private inputs
are unavailable. SQL-first integrated qualification remains mandatory at the
primary's major-batch boundary; the optional flat component cannot discharge it.

## Verification and publication

Actually executed for this document, from the preserved prep worktree:

```powershell
python -B D:\Dev\Temp\quest-story-history-boundary-20261008\pin.py
python -B D:\Dev\Temp\quest-story-history-boundary-20261008\verify.py
git diff --check
git add -- docs/persistence/economy_accounting/quest-prep/QUEST_STORY_HISTORY_RECOVERY_BOUNDARY_2026-10-08.md
git commit -m "docs: define static quest story history recovery boundary"
git push origin codex/accounting-quest-prep
git ls-remote origin refs/heads/codex/accounting-quest-prep
```

Proof is source/Git-blob/anchor/link/design/scope verification only. It authenticates
public bodies and unchanged trees, the reused prep records and the sole owned
document. **PASS**: 42 public bodies, 42 cited source anchors, 15 existing-coverage
excerpts/eight existing command references, three preserved document links, four
prep dependencies and 28 design/scope checks; whitespace checks also pass.
No build, component, SQL, server, journal, native or broad batch runs.
`verification.json`, `artifact-index.json` and `publication.json` retain exact
counts, hashes, containing commit and remote equality. Import this document
independently; closed bundles and the blocked native Goal remain unchanged.

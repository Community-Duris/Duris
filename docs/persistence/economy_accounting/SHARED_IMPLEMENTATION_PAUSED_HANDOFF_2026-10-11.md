# Shared accounting implementation: paused handoff

The user requested a complete handoff commit and a stop. This checkpoint
preserves actual unadopted implementation and unfinished qualification. It does
not mark any new plan complete. Resume only on a later user instruction.

## Authority and preserved state

- Canonical repository: https://github.com/Community-Duris/Duris.
- Published implementation base: `f215679503d787a25af52fa8103506159a50013d`.
- Local branch: `codex/accounting-review-fixes-linear-20261011fixture`.
- Worktree: `.worktrees/accounting-review-fixes` beneath the source checkout.
- Latest observed Plan 5 tip: `52ca6d9fba8da46bb63c1ce4d2da328e6bf6ab29`;
  latest world tip: `5e679b5fe0720f0506fa15a1eedd06cbf6e23fab`. Both latest
  deliveries contain documentation, not the missing runtime interfaces.
  No `codex/accounting-accel-shop` remote ref was found.
- The separate integration branch is divergent. It was inspected, not merged.
  The desktop chat connector returned `Transport closed`; owner pickup and
  current execution could not be confirmed.

Accounting stays inactive/CLOSED, coverage incomplete and release BLOCKED.
Plan 1 remains complete only at its recorded historical accepted scope.
Plans 2–4, full R1–R8, architecture and release acceptance remain open.
Do not force-push, merge, deploy, activate production accounting, modify
production data or retry the declined inactive spell-path change.

Three unrelated files remain excluded from this commit:

- `tests/async/shop_trade_physical_retention_harness.cpp` —
  `3884a11d93c1efc953e3361bcc3a219a7a5dda7cefacf5e9de56bad387506cb9`.
- `tests/async/shop_trade_publication_retention_harness.cpp` —
  `1e405a00c8081f4a2a5724aad03b3a42abc54d8dbe5cb73eed1634df8479347c`.
- `docs/persistence/economy_accounting/PLAN5_RESTORE_SESSION_NATIVE_METADATA_QUALIFICATION_2026-10-05.md` —
  `8e562fe3c24564bcd3710c4897c2a40cd92cfb501a8c6c9d9e23802d62f02f19`.

## Portable implementation

[The shared candidate patch](paused-handoff-20261011/shared-candidate.patch)
contains all 29 selected source/build files relative to the published base.
[Candidate fingerprints](paused-handoff-20261011/CANDIDATE.json) authenticate
the exact code, including its LF patch representation. The patch is preserved
as a proposal; maintained source and build registration are unchanged.

The actual implementation includes the genuine held Save/Collector/Redis
scopes, Coordinator/Currency/Authority integration, CURRENT metadata for the
selected owner families, the fifteen-family global bridge, and the active ATM
caller with checked Source/query/bootstrap admission. Root owns its real inline
layout once. Currency metadata costs are counted once; the first CURRENT
reservation precedes child metadata queries. Missing/deep erased captures and
unsupported profiles refuse rather than borrow an unrelated COPY allowance.

The caller supplies a fresh publication string. New kept/attached work transfers
its already allocated key after the last fallible admission check. All genuine
loans and locks end before the original publisher runs. Existing pending work
does not publish again; normal-return pending-health refresh is preserved.
The original publisher's allocation/throw boundary remains unqualified and
unchanged. Do not add an unproved catch-and-retry path.

Actual bounded source reviews passed for selected leaves, Root's implemented
Source/query composition, caller lifetimes/publication ordering and integration.
All six final shared formatting outputs preserve tokens, logical preprocessor
directives, macro kinds and changed-line boundaries. These are source/format
results, not compilation, native ABI, emitted-stack or whole-host 32 MiB proof.

Useful local evidence remains under `tmp/` and is not portable by itself:

- Final candidate: `tmp/fresh-bank-source-join-owner-20261011/candidate-v4-formatted-shared`,
  manifest `a0c48d43ec01d720d5ec13d5fb258f22b54b923dc105158184fe0e1ebb3c58ca`.
- Root composition: `tmp/fresh-bank-root-source-composition-owner-20261011/candidate-v2-once-only-currency-query`,
  manifest `d4d126a3480d0bc6d36f7d6f606f5e84e93ff5629aeb7052abadeeb82230d4f6`.
- Final format result: `tmp/fresh-bank-final-shared-format-root-20261011/FORMAT-RESULTS.json`,
  `65da1ae997d0a6a6fb9ecf463d508a71093c3dca5ef42c47e56a5fa389d5faf2`.
- Current working-state record: `tmp/fresh-bank-runtime-parent-root-20261011/ROOT-CURRENT-20261011.json`.

## Missing owner deliveries

Direct human authorization gives WORLD exclusive `src/world/db.c` and
`src/world/quest_mobile_native_birth.c/.h`; SHOP owns
`src/economy/shop_trade_transaction.c/.h`, `src/sql/sql_player.c` and
`src/flatfile/flatfile_item_repository.c/.h`. Preserve those leases. Root owns
unleased Zone/shared contracts, integration, registry/matrix and activation.

Seven required exports and their actual owner proofs are still undelivered:

```text
quest_mobile_native_birth_current_initial_inline_bytes
quest_mobile_native_birth_current_source_profile_query_frames
quest_mobile_native_birth_current_initial_query_frames
shop_trade_transaction_replay_current_source_frame_bytes
shop_trade_transaction_replay_current_initial_inline_bytes
shop_trade_transaction_replay_current_source_query_frame_bytes
shop_trade_transaction_replay_current_initial_query_frame_bytes
```

Root's four query calls require the actual getter plus query-accessor execution
union, including reached dependencies and numeric returns. Owner INITIAL must
be explicitly disjoint from the full Source; do not infer zero. Birth already
owns Warm. The Redis-enabled fifteen-family global census already owns store
CURRENT; the held cache scope supplies exclusion, not a second census. Keep
the existing NO_REDIS refusal. No missing export was redeclared or fabricated.
**The preserved shared candidate is therefore not yet compilable.**

Plan 5 also owes the genuine independent initialized-world verifier for the
existing `economic_sql_initialized_activation_verifier` ABI. Root supplies the
original held session and exclusions, then joins verification between
`complete_boot()` and `before_world_callbacks()`. Missing proof and failed
cleanup must retain refusal/abort obligations. No substitute callback, separate
session, new CLI or activation permission closes this requirement.

## Unexecuted recovery batch

[The recovery runner patch](paused-handoff-20261011/recovery-runner.patch)
adds exactly the three omitted native cost, coin-give and lockpick providers.
The existing fixture, assertions, flags and 20-second runtime remain unchanged.
This is 17 whole production translation units plus one original fixture.
It has no dependency on the missing shared/world/shop interfaces.

[The stopped recipe](paused-handoff-20261011/recovery-recipe/ROOT-COMMANDS.md)
preserves the explicit f215 rebind and prospective host storage guard.
Its seven-file manifest is `c506eda58390d0d6d6157eee76949add48d18f0fd7c2f4efb7009a7cb55525ed`.
Finite review remains pending. **No native/Docker batch was dispatched.**
The recipe is historical input, not directly runnable from this handoff commit:
its old HEAD/status/private-path guards must be explicitly rebound and reviewed
without weakening preservation checks. Preserve the original pinned compiler,
guest 900/host 1,020/runtime 20-second bounds, offline 8 GiB/2 CPU resource shape
and exact owned cleanup. The 1 GiB logical-artifact/512 MiB free-space policy is
prospective and sampled, not a measured ELF size or instantaneous disk quota.

The existing regression selector was actually listed successfully:
`tests/run_regression_tests.py --profile native --match native_quest_recovery_context --list`
returned exactly one existing test. No registration duration/resource edits were
made. Only after genuine native PASS should the runner be adopted and its exact
SHA string added to both `candidate_worktree_evidence.source_pins` maps:
511 to 512 entries, all original entries and policies/census unchanged.
Do not report a pure-value component as full recovery, SQL/ACK, gameplay,
activation or aggregate 32 MiB acceptance.

## Resources, cleanup and resumption

The last observed free C: space was about 2.44 GB, not a reservation. The separate
Movement replay requires its unchanged 2,013,265,920-byte guard and a fresh check.
Only four unrelated Basic Memory services were running during the read-only
Docker inspection; they were preserved. No Duris native container was started.

The user now explicitly requests finding safe deletions, including stale Duris
Docker tests. That cleanup is deferred because the same reply asks to stop.
On resume, inspect actual ownership and stale resources; preserve needed source,
dirty work, failed evidence, volumes and foreign services. Do not infer that an
unknown container or backup is disposable, or delete outside the authorized scope.

The next implementation action is to obtain the world/shop owners' exact
branch/worktree deliveries and reconcile the seven contracts with this preserved
candidate. Continue unaffected qualified work if delivery is still blocked.
Complete appropriate native/gameplay/persistence/recovery checks in meaningful
batches, then commit each solved issue and push normal milestones directly to
`experimental-accounting`. This handoff grants no acceptance shortcut.

Read `AGENTS.md`, `AI_CONTEXT.md`, `FINISH_ACCOUNTING_PLAN.md`,
`REMAINING_REQUIREMENTS.md` and the latest review checkpoint when resuming.
The canonical notebook remains the sibling knowledge overlay; its historical
checkpoint is not a new source of instructions or a factual verification date.

An immutable draft checkpoint was created in the owning overlay at
`knowledge/history/continuity/e4a9e4d8baa71a9d/a7d16f5e353e8adea5f7/ae30cf7bab70aece57af7cf3.md`.
Index/check/inspect passed with 37 notes and zero errors; verification stays
unknown. Two pre-existing oversized, unrelated notes remain unchanged.

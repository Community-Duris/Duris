# Regression journeys

One section per fixed defect: the rule the fix established, the tests that hold it, and
what those tests do not cover. Every journey uses synthetic accounts, a private world and
temporary state, and never reads the checkout's `.env`. A MariaDB journey needs
`TEST_DB_HOST`, `TEST_DB_USER` and `TEST_DB_PASSWORD` for a disposable loopback server
(`tests/async/with_disposable_mariadb.sh CMD...` supplies one) and creates and drops its
own schema. Flat-file journeys take a server built with
`make -C src PERSISTENCE_BACKEND=flatfile DMS_BINARY=/absolute/path/dms_flat`.

Four of these sections note that the full-server path was never driven under sanitizers; no
journey does that yet.

## Combined log-review integration qualification

The combined server/world/schema at `8b2681862` incorporates master `c6a95a470`
and PRs #70 → #73 → #74 → #75. The following commands passed on that stable tree:

```sh
make -C src -j16
make -C src pfile -j16
make -C migrations/tools all affects pfile_converter -j16
make -B -C migrations/tools all affects pfile_converter -j16
./scripts/format.sh --all --check
make test-all -j16 TEST_JOBS=16
make test-db
python3 tests/async/test_documentation_contract.py
npm test --prefix site
```

Results: **736/736 regressions**, 591.84 seconds; **77/77 database legs** in one
707-second run; **14 documentation checks**, **15 site tests**, and formatting
of all 1052 tracked C/C++ files. Every artifact/guild gameplay variant passes:
Summon Ghasts/boons, crash before artifact save, crash after artifact save, crash
after history commit, blocked shutdown and actual PvP/restart. Standard gameplay
also verifies canceled blocked copyover, successful drained copyover and clean
restart. Native/SQL checks retain the full ES-01–ES-05 acceptance matrix; the
following sections describe their individual boundaries. Final logs are under
`bin/analysis/review-final/`, including the build, pfile, migration tools, formatting,
regression, database and site output. Per-database-leg output is in `bin/tests/db/`.

This run includes the corrected connection observer test from `d4fe27272` and
the MySQL/MariaDB kingdom-interface repair from `b6a8ecb4c`. The latter also
passes actual MySQL 8.0.46 server/pfile/migration-tool builds, the flat-file boot
preflight, and `make security-check`; its before/after script reports **1/1
catches**. See the client-interface section below for the proof boundary.

Earlier qualification at `42b7afb17` passed 736 regressions in 567.56 seconds
and 77 database legs in 710 seconds, with logs retained in
`bin/analysis/integrated/`. The corrected observer's separate journey and
deterministic before/after probe remain there too. The newer full run above
supersedes that aggregate. Final handoff commits change documentation only.
No production deployment, historical data repair or owner-deferred investigation
is claimed by this qualification.

## Statistics across restarts and interrupted output

Issue #56 (LR-01): new maintenance work no longer reuses port/tick-derived IDs.
The original captured request survives retry; cursor continuation still prepares
the next page. Worker execution gets its own time budget after queueing and
durable-state sync. Daily
statistics append repairs a matching partial row and repeats a failed sync even
when the full row is present, preserving all existing consumer columns.

`python3 tests/async/test_maintenance_scheduler.py` fails against `7a2a63447`
because retry changes the captured values, and passes with the repair. It also
checks new identity after a fresh same-schedule boot, cursor continuation,
acknowledged/pending completions, old state layouts, bounds, and state-write failure.

`tests/async/with_disposable_mariadb.sh bash tests/async/run_statistics_mysql.sh`
links the real scheduler and repository against the full disposable schema.
It compares SQL and file values and multiplicity for same-day fresh boots,
identical samples, legacy file IDs, lost scheduler state, local midnight,
slow state sync, rollback, lost commit acknowledgement, repeated file failures,
partial writes, and failed file sync. The slow-sync case fails before the
deadline adjustment. Corrupt partial data and conflicting complete rows return
`EILSEQ` without rewriting the file. Process exits before completion persistence and pending
completion delivery are replayed from v2/v3/v4 state using legacy work IDs.
Every recovered sample retains its original timestamp and eleven counters;
an invalid state produces no output. All cases pass locally. Capture fixtures
supply counters instead of booting players; no production history is rewritten.
The random identity and consistent-restore contract is documented in
[configuration](../operations/CONFIGURATION.md).

The full gate also exposed a flight-journey evidence bug: address retention can
archive earlier boots' status logs before the final timing count. Commit
`81b8350e1` reads the archived records too; it retains the route and boot-count
assertions and changes no flight behavior.

The repaired source is `1e735655e`. Full-tree formatting and all 15 site tests
pass. `make test-all -j16 TEST_JOBS=16` passed 727/727 checks in 700.33 seconds,
including the flight journey; `scripts/check_tests_catch.sh 81b8350e1 1e735655e`
reports that the changed scheduler regression catches the pre-fix behavior.
`make test-db` passed all 75 isolated database checks in 459 seconds, including
the statistics matrix and the MariaDB flight journey.

## Kingdom persistence interface across SQL clients

PR #70's hosted MySQL 8 build exposed a MariaDB-only forward declaration in
`kingdom.h`: its `st_mysql *` callback could not be assigned to the public
`MYSQL *` callback used by character deletion. The header now uses the existing
`sql_work` type, matching the implementation. Flat-file mode still excludes
the SQL interface. No payment, deletion or reward behavior changes.

`python3 tests/async/test_compiler_warning_profile.py` compiles the actual header
against the different public MySQL/MariaDB type declarations and a flat-file
fixture that rejects any MySQL header inclusion. The MySQL case fails before
the repair; all three pass afterward. Server and pfile builds also pass against
the actual Ubuntu MySQL 8.0.46 headers/library, extracted under ignored
`bin/analysis/review-final/mysql-client/` without replacing the installed MariaDB
development package. The MariaDB server build, real guild snapshot/payment and
character-deletion regressions, flat-file boot/shutdown preflight and
`make security-check` pass. Logs are under `bin/analysis/review-final/`.
These checks repair the build failure; they are not a new CodeQL or Trivy scan.

## Guild member deletion capture failure

Building a member-deletion snapshot temporarily removes that member and their
frag totals. An allocation failure used to leave those temporary changes in
live memory. `Guild::statements_without_member()` now restores membership and
all frag fields before propagating an exception, just as it does after capture.

`python3 tests/async/test_account_character_delete_runtime.py` executes the actual
capture method with a failing SQL builder. It fails before the repair at the
retained-membership assertion and passes afterward, alongside the existing
MariaDB and flat-file account-menu deletion cases. This allocation case does
not run a server or a real database.

## Artifact and guild rewards

Integration with master `67ca142ff` (PRs #69/#72/#76), 2026-10-14: all-level
awards and deferred task recovery coexist with immediate memory rewards. History
rejections carry their reason, full operation/parent identity and decoded entity
context through the generic repository and coordinator. SQL vetoes of current
artifact/guild state remain removed. The SQL harness checks diagnostics after
rollback for malformed payload, both identity-conflict routes, missing parent
and unsupported uncommitted v1 history; it retains v1 receipt/outbox compatibility
and actual mirror/reload/order checks.

Focused integration commands passed:

```sh
make -C src -j16
./scripts/format.sh --all --check
python3 scripts/epic_zone_seed.py check
python3 tests/async/test_epic_gain_runtime.py
python3 tests/async/test_epic_skill_level_gate.py
python3 tests/async/test_epic_bonus_hot_path.py
python3 tests/async/test_set_based_gameplay_reads.py
python3 tests/async/test_persistence_severity.py
python3 tests/async/test_persistence_log.py
python3 tests/async/test_persistence_log_hygiene.py
python3 tests/async/test_critical_command_coordinator.py
python3 tests/async/test_critical_transaction_contract.py
python3 tests/async/test_artifact_guild_transactional_cutover.py
python3 tests/async/test_artifact_guild_rewards_runtime.py
python3 tests/async/test_combat_artifact_persistence.py
python3 tests/async/test_artifact_cache_codec.py
tests/async/with_disposable_mariadb.sh bash tests/async/run_artifact_guild_schema_mysql.sh
python3 tests/async/test_documentation_contract.py
npm test --prefix site
```

The regenerated manifest changes only `src/world/epic.h`'s source hash. No payout,
group size or override policy changed. Build evidence is retained in the isolated
integration worktree's `bin/analysis/integration/build.log`. This focused pass is
not final combined-stack qualification or production rollout; full-server fault
journeys and the broad gates remain due after #73–#75 integration.

Issue #58 is locally qualified on `fix/58-artifact-guild-memory`. The
[qualification record](../ongoing-projects/log-server-review.md#qualification-update-2026-10-14)
maps the acceptance criteria to tests, records local logs and separates review
readiness from production rollout.

`python3 tests/async/test_artifact_guild_rewards_runtime.py` links real reward
capture/application, transaction submission and codec code, with controlled writer
and unrelated engine services. Three feeds change memory before any completion;
same-guild awards accumulate prestige and cross construction thresholds. Caps,
eligibility, invalid combined arithmetic, pending duplicates, late completions and
bounded history refusal/retry do not revert current state. It passes in the worktree
and fails against `0ccec836e` at the first pre-completion timer assertion.

`python3 tests/async/test_artifact_guild_snapshots.py` uses real artifact storage,
guild methods and SQL capture bodies with controlled commit receipts. It passes
current-state retry, old-acknowledgement exclusion, allocation/admission failures,
removal/recreation, bounded retry fairness, guild spending/settings and cache
generation cases. A read started after failed admission could previously publish
stale data after retry; the added case failed before retry invalidation and now
passes. These tests do not boot a server or prove SQL restart durability.

`tests/async/with_disposable_mariadb.sh bash tests/async/run_artifact_guild_schema_mysql.sh`
passed captured-history bursts, immutable replay, preservation of newer gameplay
snapshots, cap no-op, identity/malformed/missing-parent diagnostics, committed v1
receipt/outbox delivery, and history after artifact removal. The actual reconciliation
script accepts legitimate live/history divergence and detects mirror mismatches and
missing rows. The fixture-only return-type/header correction also passes on rerun.

The runtime fixture now also links the actual stone publication code to artifact
capture/application, covering offline return, derived identity, duplicate completion
and reconnect after child completion, and recovered claims without re-awarding.
The snapshot fixture passes bulk soul-reset acknowledgement and failure recovery.
`test_player_save_worker.py` injects every admission allocation failure and proves
no partial job/index survives refusal. Against `0ccec836e`, it compiles and fails
because a refused job remains queued; the repaired writer passes and replaces its
tail without allocating. Commit `dfecafbda` passes the before/after check.
The coordinator's allocation walk additionally exposed a reserved operation/fence
with no writer job when callback construction failed. `532fe0653` releases it and
counts acceptance only after dispatch; the native fault, identity, retry, ordering
and capacity tests pass.

`tests/async/with_disposable_mariadb.sh python3 tests/async/test_mysql_artifact_guild_reward_journey.py`
passes a full-world Summon Ghasts cast, three actual kills and epic boon awards.
With the artifact row locked, all live rewards apply while SQL retains the original
timer and 85/0 guild totals; health reports three pending histories. Copyover fails
and the server remains live. Releasing the writer yields +10,800 seconds, 115/1
and three consistent histories. A subsequent copyover and clean restart preserve
state without re-awarding. This is registered in the database gate. The recorded
binary predated the coordinator allocation fix; the final qualification below covers the later repairs.

The journey now also passes crashes before the artifact save, between artifact
and guild saves, and after a real history COMMIT before completion (a test-only
library interposer holds that boundary). Only the persisted prefix loads after
restart. Blocked shutdown exits in 36.65 seconds, names the unsaved artifact/guild
and all three operation/parent pairs, and reloads without replaying rewards.
The standard copyover/clean-restart leg passes again.

The shutdown test caught diagnostic formats whose `%s` substitutions the actual
persistence reporter intentionally suppresses. Transaction and snapshot categories
now carry the controlled labels; numeric fields preserve both full opaque IDs.
The severity and snapshot fixtures execute the real reporter and retain its privacy
checks. Repository diagnostics go directly to the metadata worker. An independent
native producer test drives the actual coordinator cache past 256 entries and
proves the real boon/combat/stone handlers do not republish duplicate completions.

The staff artifact sync callback also had a reproduced stale-state overwrite.
Its generation/dirty check retains and resaves newer memory, and state publication
no longer depends on the staff character staying online. Native cases and the
normal full-server command/query journey pass.

The real PvP death variant also passes: with a controlled 100-epic award and
artifact write held, it applies the feed and guild reward immediately, then saves
+720,000 seconds and 105/1. One child references its type-8 combat parent; save and
restart preserve that result. The fixture retains the genuine soul/ownership
checks and uses a free soul for this positive case.

The snapshot fixture now links the real kingdom payment/guild writers. It covers
pending-through-commit behavior, later rewards/settings/spending, old receipts,
failed admission/allocation, current-state retry, late resource changes and realm
recreation. The disposable SQL wrapper also runs it with real transactions: a
trigger fails the realm update after guild statements, both roll back, and retry
captures the latest values without another treasury debit. The optional missing
garrison table remains compatible. Writer scheduling/receipts are controlled;
this payment matrix does not boot a full game.

The expanded state-ordering fixture also runs transfer/soul changes, the actual
staff timer command and artifact-war callback, corpse moves, expiry/removal and
recreation around feeds. Real SQL drains and the production boot-row decoder
retain the final state; the domain mirror agrees. Room/inventory movement and
writer scheduling are fixture boundaries.

Final production-source qualification passed all 731 regressions in 695.94 seconds
and all six full-server artifact journey variants. The 76-leg database run passed
68 and found eight harness linker failures for the metadata-log callback; those
eight passed after the shared weak fixture adapter, as did the expanded artifact
SQL leg. This is 76 distinct passing legs across the run and reruns, not a claim
that the original make invocation exited successfully. Full-tree formatting,
maintained builds and all 15 site tests pass.

## Recovery placement and room-entry hazards

A full-server artifact journey crashed after announcing copyover completion. The
kernel address resolved to transport recovery reading an NPC whose room-entry
hook had removed it. An unchanged rerun passed: the triggering world state was
intermittent. File and buffer recovery now use the existing placement mode that
skips movement hazards and procs for restored players and NPCs.

`python3 tests/async/test_world_singletons.py` runs the production buffer restorer
under ASan/UBSan. Its room-entry adapter extracts on ordinary movement; before
the fix the next state write is a heap-use-after-free. With recovery placement,
five restore cycles preserve the NPC, stock and equipment. File/player call-site
checks accompany the native test. All six full-server artifact/database variants
then passed on the repaired source. The native test controls entry effects and
does not execute the entire room handler. `scripts/check_tests_catch.sh
15ccfdd56 07d3e2835` reports 1/1 passing after and failing before the fix.

## Epic boon completion and actual award result

After #69 removed the gain-level gate, LR-02 follows `gain_epic()`'s synchronous
result. Completed boons remain consumed, failed awards explicitly report no
points, and successful awards retain their existing feedback at every level.

`python3 tests/async/test_epic_gain_runtime.py` retains the actual boon completion,
publication, award preparation and balance paths under ASan/UBSan. The boon
fixture covers levels 1/29/30/49/50/56, ID-generation failure, balance overflow,
ledger refusal after a successful live award, non-epic EXP and duplicate/replayed
completion. Balance/revision, feedback, pending/completed state and companion
artifact/guild publication count must agree. SQL results and unrelated engine
services are controlled adapters; full-server persistence is covered separately.
The original gain-level test and its policy are superseded; the separate
`test_epic_skill_level_gate.py` still requires level 30 for skill purchases.
The three focused epic gain/skill/training tests, server build, formatting and
14 documentation checks pass on the integrated #73 tree. The disposable
`test_mysql_epic_training_journey.py --server bin/server/dms_new` also passes
Human Warrior and Illithid Psionicist HP/GMCP/purchase/save/restart coverage after
`make world`. The first attempt lacked generated world files and failed at boot;
the successful rerun and build are retained under `bin/analysis/integration/`
in the integration worktree. Final combined-stack gates remain due.

`scripts/check_tests_catch.sh 2ec958e83 5e75d025a` reports 2/4 catches: native
boon/award and teacher regressions fail before and pass after. The two database
journeys skip without explicit settings, so their catch-script status is not
proof; the separately executed disposable training journey supplies that evidence.

## Paid epic training feedback and immediate HP

LR-03–LR-05 qualify the existing level-30 teacher policy and fix two purchase
results: reaching 100 now announces mastery, and learned stats are rebuilt with
`affect_total()` before GMCP vitals and the character save. Prices, increments,
eligibility, missing-HP semantics and memory-first balance changes are retained.

`python3 tests/async/test_epic_training.py` executes the actual teacher, purchase
callback and epic transaction under ASan/UBSan. It covers 29/30/31, configured
39/40/41, class/prerequisite/funds/cap refusals, intermediate and final purchases,
repeat at mastery, and refresh/publication/save ordering. Cash, refresh, save
and client output are adapters; they do not prove the HP formula. The old
source compiles and fails at the missing mastery announcement.

`tests/async/with_disposable_mariadb.sh python3
tests/async/test_mysql_epic_training_journey.py` covers the actual paid command
and persistence with teacher 80862 in an isolated world. Before the fix, learning
Toughness 10 charged 150 epics and 1,000,000 copper but left HP at 233/263 instead
of 243/273. After it, full and injured Human Warriors and Illithid Psionicists
receive the proper bonus immediately in score/prompt and GMCP. Later pulses and
repeated refreshes add nothing, refused purchases charge nothing, and both clean
restarts preserve skills, balances, derived HP and injury. The journey is part
of `make test-db`. Evidence and binary hash are retained under ignored
`bin/analysis/epic-training/`. The server build and full-tree formatting pass;
the batch is locally qualified in PR #73. Its full database run passed 76/77;
the artifact PvP caster aborted before spell completion. The fixture now retries
only confirmed aborts, with a five-attempt bound, and the affected PvP rerun passes.
The aggregate is 77 distinct passing database checks; the original make command
exited nonzero. Server sources match the passing 733-regression gate.

## Zone average sentinels and missing help searches

Review repair after parent integration `b89150407`: a 65,500-byte name overflows
basic `do_area()`'s old `strcat` under ASan. The response now caps the name only
after reserving the complete average, optional staff zone ID and danger warning.
`python3 tests/async/test_zone_average_display.py` passes ASan/UBSan for short and
oversized plain/colored names, numeric/sentinel values, staff no-argument output
and the +15/+16 warning boundary. Original failure output is retained in the top
checkout's `bin/analysis/area-before.log`.

Seadog help now distinguishes the aboard owner's persistent +2 speed from the
panel actor's order/display maximum. An isolated native probe exercises actual
order/max-speed/aboard-owner code and the tick clamp: a non-owner's bonus is lost,
an aboard owner's persists, and a non-Seadog sailor cannot order that owner's
full maximum. Display's actor selection, innate availability and seller's 10%
bonus are traced in production source. No sailing mechanic changes.
`python3 tests/async/test_flatfile_help_catalog.py` and
`python3 tests/async/test_help_import_live_parser.py` pass. The disposable
`test_information_cache_journey.py --backend mariadb --server bin/server/dms_new`
passes corrected Seadog text through actual import/cache/player help alongside
Web Spray and Quickstep. Its initial adapter error was corrected to inspect the
client transcript, not the timing return. Server build and 14 documentation
checks pass. Integration build/help/probe evidence is under
`bin/analysis/integration/` in the isolated integration worktree. Final combined
stack qualification remains due; the following broad results are historical.

LR-06's display now preserves -2/-1 internally while rendering `unknown` and
`not applicable`. `python3 tests/async/test_zone_average_display.py` executes the
actual basic `do_area()` branch under ASan/UBSan, covering real averages, zero,
empty/unknown sentinels, +15/+16 danger boundaries, subsequent value changes and
staff no-argument output. It compiles before and fails on the negative display.
Padding/morph adapters and omitted staff search subcommands bound this proof;
original-zone attribution and its catalog investigation remain open.

LR-08 supplies searchable Web Spray, Quickstep and Seadog topics. The existing
flat-file help test proves rendering; the MariaDB information journey dry-runs
and imports into its disposable schema before actual player help queries and
cache publication. It retains the existing Quick Step title and adds no import
or redirect mechanism. Both focused checks pass. PR #74 is ready with 734 regression and 77 database
passes in single full runs. Direct base-data lookup fails specifically at the
missing Web Spray topic and passes after; the before/after script also reports
2/2 catches, with its information-journey summary ending in an unrelated runtime
warning. The original zone attribution remains open.

## Damage death results, healing posture and graph distance

**Limited-zone spell callers, 2026-10-16 (#106).**
`python3 tests/async/test_graph_distance_runtime.py` now also executes the actual
Teleport, Depart, and Etheric Gust bodies with the real graph and room structs.
Each previously returned unconditionally in virtual rooms 5700–5999, rejecting
even an adjacent destination. Each now rejects only a negative distance result,
retaining the five-room cap. Self/distinct targets, zero/one/five/six/far distances,
unreachable destinations, ordinary zones, combat cleanup/preservation, wait, and
queue reuse pass under ASan/UBSan. Restoring each original caller separately
fails at the adjacent-room assertion. RNG, eligibility, presentation, movement
endpoints, and wait are adapters; no live cast, production rollout, or historical
attribution is claimed. The development server build and eight related focused
zone/movement/lifecycle/latency tests pass. Evidence and remaining original
acceptance are in [the continuation plan](../ongoing-projects/plan-issue-106.md).

The legacy physical `damage()` wrapper now stops a skill after either
participant dies, matching its spell branch. Previously simultaneous death
fell through into fighting/counterattack setup and `cleave()` could request a
short affect for its dead NPC target. `test_damage_death_result.py` executes the
actual wrapper and cleave under ASan/UBSan, controlling lower-layer death
outcomes and unrelated eligibility. It proves live effects, victim/both/actor
death stopping, and fresh storage reuse; real scheduler/cancellation harnesses
retain dead-owner rejection, extraction cleanup and owner reuse. This is a
producer reproduction, not a full lower-layer combat journey or exact
attribution of the original logged NPC schedule.

The existing posture test now executes `calculate_ch_state()`/`update_pos()`.
An upright humanoid can remain standing at -3 HP/incapacitated and heal to -2;
the producer now selects sitting/resting instead of standing/resting. Native
cases preserve lower postures, normal standing and standing sleep, then verify
stand/rest state and regeneration-start/feedback observations. Random falling,
combat/wake and unrelated services are adapters. The original player's precise
transition remains unknown.

`test_graph_distance_runtime.py` runs the actual `how_close()`/queue/mark bodies
and edge macros. Maximum distance now precedes target acceptance: distance four
with limit three, and distance one with limit zero, are refused. Exact/below/
above, negative limits, invalid rooms, door eligibility, ocean/NOWHERE exclusions,
unchanged room cap, one diagnostic per bounded exit, queue cleanup and reuse
pass. Allocation/logging are adapters. Original reported caller/source/target
attribution remains unavailable.

All three native checks fail on `b2982e75e` and pass on `f8e302270` in the formal
before/after script (3/3). Focused sanitizer runs, command-latency/budget and
real scheduler/cancellation checks pass. Final qualification on `7ab77e895`
passes all 736 regressions in 869.90 s and all 77 database checks in one 775 s
run, including the full connection journey and every artifact/reward variant.
The separate native cure-spell producer probe also fails before and passes after;
its heal arithmetic is an adapter, while cure/update/stand/rest are actual code.

## Crumbling equipment removal

PR #51's review reproduced a pre-existing use-after-free when `obj_to_char()`
destroyed `ITEM2_CRUMBLELOOT` equipment during removal. `remove_item()` now removes
item effects before transfer and reports extraction to its callers. Single-item
removal, `remove all`, and automatic replacement never inspect or message the freed
object; a replacement can still occupy the vacated slot. The duplicated battlemage
robe cleanup now runs in the shared removal path before the object can disappear.

`python3 tests/async/test_remove_crumbleloot.py` compiles the production public
remove command, removal/replacement helpers and inventory-transfer function under
ASan/UBSan. It covers crumbling and ordinary items, trusted-player exemptions,
invisibility/enchantment cleanup, and the robe's spell cleanup. The fixture uses
real heap deletion at extraction; the original code reports heap-use-after-free
in `remove_item()` through `do_remove()`. World messaging, equipment detachment
and spell bookkeeping are fixture boundaries, not a complete server journey.

## Learned flight network

Issue #40 replaces fixed ordinary routes with 25 per-character discoveries and
500-copper-per-room tickets. Scorched and Snow Ogres departures remain epic-only,
with their original level-51 gates, 200p/50p fares and one-way service. Discovery
EXP uses the current gain cap times discovery count divided by 27, then the normal
modifiers and caps. The discovery mask and EXP share the existing status checkpoint;
migration 0038 and snapshot schema 8 preserve both backends and old supported saves.

`test_flight_network.py` executes the production catalog, BFS and command handlers
under ASan/UBSan. It covers all paths and wrapped edges, first/repeated discovery,
distance ordering and ties, exact half-platinum fares, payment/creation failures,
busy carriers, transferred and malformed tickets, epic exclusions, the new level-one
Stronghold trip, anywhere/in-flight listing, and unavailable map positions.
The PR #51 review additions check sleeping/paralyzed and controlled-carrier redemption and require every
empty-return event to have a null passenger.
`test_world_singletons.py` retains its repeated initialization, duplicate passenger
and item preservation, occupied recovery, empty return, and older-record checks.
The capture/codec, flat-file repository and SQL save/load fixtures exercise the new
field, older versions 1/3/5/7, unrelated partial saves, account siblings, rollback,
retry and the actual pwipe reset/query. The final audit additionally reads valid
checksummed files for all four old versions through the real flat-file reader under
ASan/UBSan, verifying an unsigned zero discovery mask. The EXP fixture checks capped/immortal
characters and credited amounts across level advancement.

`test_flight_network_journey.py` runs the complete world with synthetic characters.
The default is flat-file; the database gate passes `--backend mariadb --server
bin/server/dms_new` under `with_disposable_mariadb.sh`. Each backend discovers all
25 ordinary stops, checks a 39-room flight costing 19.5p, preserves an occupied
flight and an empty returning carrier through copyover, takes both restricted
trips, and verifies discoveries/EXP after a crash and a normal logout/login. It
also kills the seven named dragons and checks their corpse objects. The SQL leg
imports the maintained help sources; both clients read the same FLIGHTS guidance.
Both backend journeys also apply and verify minor paralysis on the actual carrier,
then use ordinary `give` to prove ticket ownership and wallet retention. A separate
flight logs out immediately after landing and verifies that the empty carrier
returns home and resumes service without a reboot or copyover to rescue it.

Fixtures supply light and detect invisibility because the existing epic caster can
turn invisible; their epic departure pads are quieted to isolate ticket access from
unrelated combat. A small purse avoids the normal accidental coin drops caused by
carrying too many loose coins. A diagnostic quiet-room copyover preserved 1000p;
the overfull-purse loss began during movement before copyover, so that mechanic was
retained. Neither transport recovery format changed: file copyover remains version
16 and Redis generations remain schema 13. Full servers use development builds;
sanitizers cover the isolated native fixtures.

Validation on 2026-10-12 at `11354ecd1`: `make test-all -j16 TEST_JOBS=8` passed all
720 scripts (539.50s); `make test-db` passed all 73 legs (361s). All 603 routes loaded;
one MariaDB run prepared paths in 483–645 ms across five boots. Full-tree formatting,
maintained pfile/migration-tool builds, and the 15 site tests passed. The before/after
check against `b79c08eef` catches the flight command and journey changes; four test
maintenance cases pass on both revisions. The busy-carrier case was also observed
failing before its focused repair. No production data or running game was used.

PR #51 review qualification at `68b638d9e`: the final `make test-all -j16
TEST_JOBS=8` passed 721 scripts (672.99s). All 73 database legs passed (443s),
and the affected MariaDB flight journey passed again after the final carrier guard
refinement. `scripts/check_tests_catch.sh 6df413724 68b638d9e` reports that all
three changed regressions pass after the repairs and fail on the reviewed code.
Full-tree formatting, security checks, and the documentation's 15 site tests passed.

## Login statistics retain event-time identity

`test_session_audit_snapshot.py` drives the real submit and codec path, mutates the
live account and level after queueing, and verifies unchanged identity, version-1
replay and malformed-payload rejection. `run_session_audit_schema_mysql.sh` writes
through the real typed repository and runs the SELECT-only exporter before and
after promotion, transfer and deletion. It also covers alts, staff, legacy NULL
snapshots, today exclusion and UTC/Sydney boundaries. The temporary user-unit
journey `run_login_stats_service.py` checks ordering, first-run directory creation
and recovery after the first query fails. No test changes the live deployment.

## Monster shrug and Rrakkma

The seven restored monster shrug values leave playable vampires at 40 through a
dedicated `innate.shrug.PVampire` key; other vampire settings still share their existing
race name. Rrakkma reads `innate.rrakkma.stepPct`.

`python3 tests/async/test_racial_shrug.py` executes the real lookup, race/innate tables,
and resistance calculation under ASan/UBSan with the shipped properties. At level 50,
playable vampires retain 34 resistance and monster vampires have 54. Two grouped
Githzerai have 19 resistance with step 5, 14 with step 0, and 17 with step 3. The
playable-vampire and zero-step assertions both failed before their fixes. Unrelated
services are stubbed; this is not a live combat journey.

## Property saves, reloads and key bounds

Save failures preserve the original file and the saved-value baseline. Only a successful
replacement updates the baseline and logs success. Reloads install and apply the complete
new table only after successful reading and validation, freeing the previous keys; failed
reloads leave values and character caches unchanged. Startup refuses an unreadable or
malformed configuration. Keys up to 127 characters fit; longer keys are rejected whole.

`python3 tests/async/test_issue263_property_reload.py` executes the real property commands
under ASan/UBSan in both SQL and flat-file builds. It checks 127-, 128-, and 200-character
keys; missing sources and an unopenable temporary file; partial reads; write/flush failures
using `RLIMIT_FSIZE`; separate source/target close failures; failed rename; malformed
properties and capacity overflow. Failures must preserve the file, active values and
`diff`/`revert` baseline, clean incomplete temporary files and notify the operator. The
success path checks save/reload/revert and the existing loaded-player/NPC stat refresh.
These scenarios use disposable files and injected I/O failures, not the running server
or its configuration. The pre-fix code overflowed under ASan, crashed on failed opens,
lost active reload values and falsely reported successful saves.

## Web Spray: targeting, refresh, recovery and mounted travel

Driders use `innate web_spray <target>` from level 31. A successful hit replaces the
30-second affect and its expiry event. The affect stores the drain amount, and its
creation/removal arms/cancels the drain for both fresh hits and player/NPC recovery.
Both NPC serializers update short affects from their scheduled expiry before saving,
so recovery preserves the remaining duration.
The final ordinary movement pass checks each living rider and mount that changed rooms;
the temporary visibility placement and self-loop exits give no extra escape rolls.

```sh
python3 tests/async/test_web_spray.py
```

The ASan/UBSan harness executes the production innate parser, dispatcher, target selection,
cooldown, spray, affect lifecycle, player affect codec, both NPC serializers/restore blocks and final
movement block. It checks an explicit target different from the current opponent with a
bystander named `spray`, hits at 0 and 29 seconds expiring at 59, saved drain rates and
remaining time, one drain after restoration, both mounted control directions, failed moves,
self-loops, uncarried/dead partners, defence outcomes and vitality clamping. Scheduling,
combat outcomes and room placement are controlled fixtures; it is not a live copyover test.

## Argument tokenizer: unterminated quoted words

`lohrr_chop()` resets its output pointer when an unterminated single quote makes it
restart as an ordinary word. Previously it appended the second attempt to the first,
duplicating text and overflowing a command-sized output buffer on long input.

```sh
python3 tests/async/test_argument_tokenizer.py
```

The native ASan/UBSan test uses the real tokenizer and a maximum-length unterminated
word, which reproduced a stack-buffer-overflow before the fix. It also checks the
unquoted fallback, complete quotes, whitespace, empty input and the returned remainder.
This tests the shared tokenizer directly; the randobj harness separately drives long
unterminated words through the command.

## Randobj: invalid arguments and missing generated objects

`randobj` validates the whole subcommand before creating anything. Equipment, material
and stone failures are checked before inventory placement or `stat`; a missing blank
stops a batch and preserves earlier items. Recipe, mob and sample-name generation also
stop on failure. Explicit levels are passed to the generators without changing the
staff character.

```sh
python3 tests/async/test_randobj_command.py
```

The native ASan/UBSan harness calls the real command with the real tokenizer, wear words
and slot tables. Recording generators cover every noun and wear group, defaults, both
numeric forms, bounds, extra words, trust, null returns and mid-batch failures. It also
checks `eq list`, single-item stat output, batch totals and unchanged map/zone operations.
It does not prove the live command's generator output; the random-equipment golden harness
and the local gameplay check recorded in the ongoing plan cover those separately.

## Randobj: recipe totals count delivered scrolls

`create_recipe()` reports success only after placing a scroll in inventory. Recipe
batches use that result for their success lines and total, so rejected targets no longer
inflate the count. The attempt limit, eligibility rules and target cleanup are unchanged.

`test_randobj_command.py` compiles the real recipe function alongside the command. Under
ASan/UBSan it checks full containers/storage, prohibited affects in all four bitvectors,
combat/spell pulse affects and a false crafting-availability result. All-rejected, mixed
and single attempts check inventory, cleanup and exact output. The all-rejected case
failed before the fix. The availability gate itself is controlled by a test stub; these
checks verify how the recipe function and command handle its result.

## Randobj: stat output selects the new item

Single-item equipment generation passes its object directly to `do_stat()`'s renderer.
Previously it passed the generated keywords, whose first word is always `random`; the
command parser looked up that word and could display an older carried or worn item.
Ordinary `stat` commands retain their existing lookup.

The same native command harness asserts the exact object passed for display after several
items with identical keywords. The local gameplay check also inspects successive weapon
and ring stat blocks; the harness uses a recording stat function, not the full renderer.

## Random equipment: revealing a foreign weapon spell

`identify_random()` bounds its search by the 61-entry spell table. A weapon with a spell
outside that table keeps its name; the old 62-iteration search read past the table.

```sh
python3 tests/async/test_random_equipment_golden.py
```

The native ASan/UBSan harness asserts that a weapon with `SPELL_BLINDNESS` stays unchanged,
in addition to comparing every existing true-name case with the frozen fixture. This
covers the real naming function, not the live reveal-true-name spell command.

## Achievement zones: quest givers, tie order and area names

`achievements zones` lists each zone where the character has completed a quest. A quest
belongs to the zone with the highest first vnum (zone number × 100) at or below its giver's
vnum, which is the area its giver comes from: an area's mobs run past its first hundred
vnums and past its top room. `zone_for_giver_vnum()` and `scripts/zone_story_quest_catalog.py`
use the same rule, so the checked-in catalog snapshot matches the runtime catalog. The
heavens givers (zone 0) stay on zone 1, because the catalog needs a positive zone. Zones
with equal counts are ordered by the name a player reads: color codes are skipped as
`strip_ansi()` skips them, and case is ignored. `achievements zone <area>` finds an area by
its name as well as by its number.

```sh
python3 tests/async/test_zone_story_quest_production.py
python3 tests/async/test_zone_story_quest_production_catalog.py
python3 tests/async/test_zone_story_quest_feature.py
python3 tests/async/test_achievements_zone_lookup.py
```

The production harness maps givers past a zone's first hundred vnums and past its top room.
The catalog test checks a Winterhaven giver and a Tower of Darkness giver in the real world
files. The feature harness ties seven zones with real colored names. The lookup test runs
the command's lookup with the real `is_abbrev()`, `strip_ansi()` and `skip_spaces()` on the
text `one_argument()` hands it. No test completes a quest on a running server.

## Addresses kept 30 days after their last use

ADR 0003 keeps a network address at most 30 days, except on the ban list. The hourly
`address_retention` maintenance job deletes `account_ips` and `account_login_history`
rows last written over 30 days ago and clears the address of older `log_entries`,
`ip_info`, `player_data` and `account_characters` rows, at most a row budget per table a
run. An account keeps each address's last use: a login rewrote the whole list with the
save's time, so an account that logged in once a month kept every old address. A login
now drops an address past 30 days and leaves the others' times alone. The same job moves
the live logs into `logs/old-logs/<date>/` once they are a day old and removes each archived
file and core dump 28 days after its last write (a file holds at most about 25 hours of
lines, so none reaches 30 days): the launcher archived only between runs,
by the archive's age, so a server kept up by copyovers kept its logs for good and a 40-day
run's first lines lived 70 days. The server clears `lib/etc/hosts` at a cold boot and
removes a connection's files when it closes, and a reverse-DNS lookup that answers after
its connection closed writes nothing (it used to write its file back after the close).

```sh
tests/async/with_disposable_mariadb.sh python3 tests/async/run_address_retention_journey.py bin/server/dms_new
python3 tests/async/test_log_retention.py
python3 tests/async/test_hostname_files_journey.py
python3 tests/async/test_hostname_lookup_cancel.py
python3 tests/async/test_maintenance_scheduler.py
```

The MariaDB journey logs in on a real server, then runs the job against the same database
without `account_login_history` and with it, with a row budget of one. The log test runs
the real `expire_log_files()` on file trees of known ages. The hosts journey boots a
flat-file server over stale files and closes a connection; the lookup test runs the real
lookup code under ThreadSanitizer with `getnameinfo()` held until after the close. The
scheduler test loads state files of versions 2 and 3, which held 11 and 12 jobs. No test
waits for the hourly slot on a running server, and the flat-file backend's address lists
are not pruned.

## Area-authored coin piles

Get, take and put handle any `ITEM_MONEY` object by type, whatever its vnum, and add a
picked-up pile to the wallet at once; the player's save writes the wallet. The reported
pile was `areas/obj/library.obj` #402013, ten platinum that a `P` reset hides inside
statue #402001. A pile kept in a container is an ordinary item: the save writes it with
its amount and claims it, and a load reads that amount back. The amount an older server
kept in the pile's custody row (`coin_payload`) is no longer read, and a pile such a
server's coin transaction spent stays spent. Only the pile made from a death wallet must
still be vnum 3.

```sh
python3 tests/async/test_area_coin_pickup.py   # --server /absolute/path/dms_new reuses a flat-file binary
python3 tests/async/test_take_coins.py
python3 tests/async/test_currency_in_memory.py
python3 tests/async/test_flatfile_item_repository.py
python3 tests/async/test_live_item_movement_contract.py
bash tests/async/run_experience_trophy_mysql.sh
```

`test_area_coin_pickup.py` boots flat-file servers with the real statue and money
prototypes under new vnums. Each fresh character searches the statue, picks the coins up
with one of `get coins statue`, `get all.coins statue` and `take all statue`, and saves;
the test checks a durable ten-platinum wallet, an empty statue and no credit on a second
pickup. The player-load harness in `run_experience_trophy_mysql.sh` loads a pile whose
custody row holds an older amount and checks that the saved amount wins.

## Bulletin boards

`board_info[]` in `boards.c` is the only source of the `board` special:
`initialize_boards()` gives it to every row's object, once, at boot. Each row names an
object in the world `make_all` builds whose keywords include `board` or `bulletin`. No
zone loads two different boards into one room, because `write`, `read <n>` and
`remove <n>` name no board and the first board in the room takes them. A board answers
for its own row, and only while it stands in the character's room. A headline keeps at
most 70 characters, and a write without one takes no message slot. Every message loaded
from a board file gets a slot of its own, including one whose body was aborted. A save
writes `<file>.tmp` and renames it over the board file. A board file cut short or corrupt
is logged and resets that board, and the boot goes on.

```sh
python3 tests/async/test_boards.py
python3 tests/async/test_spec_assign_vnums.py   # specs.assign.c assigns board nowhere
```

`test_boards.py` checks the table against the area and zone files `areas/AREA` lists. It
then compiles `find_slot()`, `find_board()`, `board()`, `Board_write_message()` and the
save, load and reset functions from `boards.c` into a harness under ASan and UBSan. The
cases are:

- room 1213's two boards;
- a carried board;
- a headline at the end of a 1024-byte command line;
- a blank headline;
- an aborted message reloaded after another board's message;
- a file cut short;
- a file whose heading length is 0 under a pointer;
- a save whose copy cannot be opened.

It does not drive the board commands through a running server, and it never makes the
`fsync` or the `rename` fail after the copy is written.

## Casts that run behind the event pass

A spell with a cast time above four pulses is cast in segments of up to four, and each
segment was scheduled from the tick the one before it ran at, so every callback the event
pass ran late pushed the cast back by its lateness: a 12-pulse cast whose three callbacks
each ran two pulses late took 18. Each segment is now due at the tick the one before it
was due plus its own length (`spellcast_datatype.due_tick`, advanced by
`schedule_spellcast()`, and set by `MobCastSpell()` for the segment it schedules itself),
so a late callback shortens the next segment, never below one pulse: a cast finishes at
its cast time plus the lateness of its last segment, or later only by that minimum. A
cast with no lateness takes exactly its cast time. `DelayCommune()` and the casting
display keep the nominal segments. Landed in `7ef76523e`.

```sh
python3 tests/async/test_cast_lateness_runtime.py
python3 tests/async/test_spell_schedule_failure_runtime.py
python3 tests/async/test_death_field_runtime.py
python3 tests/async/run_cast_timing_probe.py --server /absolute/path/dms_flat [--budget-usec 2000]
```

`test_cast_lateness_runtime.py` compiles the production `schedule_spellcast()`,
`event_spellcast()` and `MobCastSpell()` and runs each callback at its due tick plus an
injected lateness: casts of 1 to 20 pulses with none take their cast time in segments of
four; the item's 12-pulse example finishes in 14; lateness before the last segment is made
up; after a stall the minimum leaves one pulse per remaining segment; and a mob's cast
makes up its lateness as a player's does. The other two compile the helper and keep
rejection and NPC casts as they were. The probe is the measurement on a real full-world
server, not a test, and is not in `make test-all`: it reads each cast's segments from the
`PLAYER EVENT TIMING` trace and prints how late it finished. Not covered: a cast under real
lateness in a test leg.

## CHAOS kit for centaurs and driders

The CHAOS kit judges each body slot for the level-56 character it is for. Creation builds
the kit before the character's first level, and the horse body and spider body slots come
from racial innates that unlock at level 1, so a new centaur never got horse-body item
87585 and a new drider never got spider-body item 85714.

```sh
python3 tests/async/test_chaos_kit_runtime.py
```

The test compiles the production kit helpers under ASan/UBSan and checks that a level-0
character's slot is asked about at level 56 and that its own level comes back unchanged.
On a disposable copy of the development database a new centaur warrior got its tail item
but not 87585, and with the fix a new one got both. Not covered: a journey that creates a
centaur or drider and reads its saved kit.

## Coin put after an auction listing

An auction settlement advances the ownership revision of both the source and the
destination, and the completion publisher in `src/economy/auction_transaction.c`
publishes both, keeping a newer source revision that is already published. Publishing only
the destination left a seller's next `put all.coins bag` refused until a full relog. A
rejected auction transaction publishes nothing. Money itself moves in memory: the listing
fee leaves the wallet when the listing is submitted, and coins go into the bag at once.

```sh
python3 tests/async/test_auction_ownership_publication.py
python3 tests/async/test_flatfile_auction_coin_put_journey.py   # --server reuses a flat-file binary
```

The first runs the real publisher with the real ownership runtime and codecs under
ASan/UBSan: rejected settlement, listing, claim, and a newer source revision kept. The
journey puts food in a bag, acquires ten platinum, lists another item, puts the remaining
coins in the bag at once, repeats the transfer, and checks the exact balance after a full
logout and login; it saves before it reads the wallet from disk. `--expect-regression`
only fits a server from before the persistence reset. Live play ran on the flat-file
backend only.

## Command log: one-letter commands

`cmdlog()` recorded a command only when its second byte was not the terminator, so a
one-letter command (`n`, `s`, `k`) never reached `cmd.debug`, and an empty line read the
byte past the terminator. It now tests the first byte: a one-letter command is kept, and
an empty line leaves no entry.

```sh
python3 tests/async/test_command_log_ring.py
```

The harness runs the real `cmdlog()`. No test types a one-letter command on a running
server.

## Connections before an account login, and the trusted proxy

One address may hold at most `MAX_LOGIN_CONNECTIONS_PER_ADDRESS` (8) open connections
that have not logged in to an account: negotiating TLS, waiting for the WebSocket
handshake, at a prompt to log in, create an account or reset its password, or closing.
`new_descriptor()` closes the next before setting it up. A connection silent at one of
those prompts is closed after `LOGIN_PROMPT_TIMEOUT` (120 s). The addresses of one IPv6
/64 count as one client (`same_client()`). A connection from `DURIS_TRUSTED_PROXY_IP`
counts under the address its PROXY header names; a WebSocket connection from it without
one takes the last `X-Forwarded-For` entry, the one the proxy appended; any other has the
proxy's address, shared by its clients, and is not limited. The listeners are IPv6
sockets, so an IPv4 proxy arrives as `::ffff:a.b.c.d`; `proxy_peer_is_trusted()` matches
that against the IPv4 setting. Before that the proxy was never trusted, and one website
login closed every other one in progress as a stale connection from the same address.

```sh
python3 tests/async/test_connection_limit_journey.py   # builds or reuses a flat-file server
python3 tests/async/test_websocket_protocol_contract.py
```

The journey boots a flat-file server with the WebSocket listener on and `127.0.0.9` as
its proxy, binding client sockets to loopback aliases: nine telnet and nine TLS
connections from one address (the ninth refused); eight connections that each entered a
name, then a ninth, refused; a login from another address; PROXY-header connections for
two clients and for addresses of one IPv6 /64, where one of eight PROXY-named connections
completes a handshake with a forged `X-Forwarded-For` and the ninth is still refused;
nine telnet connections from the proxy itself; two website logins with different
`X-Forwarded-For` addresses (one forging the other's in front of its own); connections
silent at the account name and password prompts, closed between 115 and 135 s, while a
website client that sends a login every 25 s is still open at 130 s; a banned address;
and TLS connections to a full server. It takes about three minutes (173 s in a loaded
gate).

The LR-11 extension measures another 2,001 sequential plain/TLS/WebSocket
attempts and separate incomplete handshakes on the isolated server. Its focused
qualification took 80.835 s (24.75/s), kept descriptors at 10 before/after with a
peak of 26, and grew peak RSS by 1,012 KiB. Process CPU was 9.02 s; 2,563 lines /
261,011 bytes were logged, with no pulse above 250 ms. An authenticated account
menu responded throughout (maximum 216.93 ms) and the burst address could log in
afterward. Held WebSocket/TLS handshakes expired at 27.16/37.54 s. Assertions cap
resource/log growth for the stream and verify cleanup and reconnect. A fixture
context-manager error after the first resource measurement was corrected; the
focused rerun passes. No admission/rate-control source changed; these local
measurements are not an unlimited-load guarantee or privileged host audit.

## Copyover state path and failure output

`COPYOVER_STATE_FILE` selects the state file shared by capture, listener-header
inspection, recovery and cleanup. Unset or empty keeps `copyover.dat`. Docker sets
`/var/lib/duris/copyover.dat`; the image provisions `/var/lib/duris` as UID/GID 10001 with
mode 0700. The sibling `.tmp` file uses the same directory and is renamed after all saves
complete. A failure notice enters the descriptor output queue, so the live output loop
frames it for Telnet, MCCP, TLS and WebSocket instead of writing plaintext into a
compressed or encrypted stream.

```sh
python3 tests/async/test_copyover_failure_runtime.py
python3 tests/async/test_copyover_save_guards.py
python3 tests/async/test_copyover_custody.py
python3 tests/async/test_telnet_output_runtime.py
python3 tests/async/run_copyover_runtime_journey.py /absolute/path/dms_flat
# Start the fixture as root; it drops the server to UID/GID 10001.
python3 tests/async/run_copyover_runtime_journey.py /absolute/path/dms_flat --nonroot
```

The journey saves a synthetic player, promotes its offline snapshot to staff and restarts
with the state file's parent directory missing, so the real copyover command fails. The
same client then runs `look` and an acknowledged `save`, which proves transport and the
save worker resumed. With the directory created, a second attempt publishes, execs the
staged binary, recovers and removes the state file, and `look` and `save` run on the
original socket. MCCP is decoded with zlib, and plaintext in the stream fails the test.
It runs plain and MCCP, as the ordinary and the non-root fixture.

Not covered: the complete release image (the journeys use a minimal world), and full TLS
and WebSocket player sessions across a failed or successful copyover; the failure helper
is tested with separate TLS and WebSocket descriptor queues. A disconnect after a
successful copyover, once seen with a full-world Docker server and TinTin, was never
reproduced.

## Corpse bulk loot

`get all corpse` captures the corpse's display name for the operation. When the first
item is taken, the looter and the room see the start. At the end the player sees the
sorting line, the haul (only delivered items and the coin amounts actually taken), then
partial and failure notices. NPC and player corpses present the same way. The haul runs
at once, in memory: it selects the items, applying the carry count and weight limits as it
goes, then takes each one. Each selected coin pile gives up only the denominations it held
when selected, and an item an earlier pickup moved or destroyed is reported as no longer
available. The next save of the player and of the corpse records where the items went.

```sh
python3 tests/async/test_corpse_haul.py
python3 tests/async/test_bulk_get_publication.py
python3 tests/async/test_money_carry_count.py
python3 tests/async/test_take_coins.py
python3 tests/async/test_get_trap_lifecycle.py
python3 tests/async/run_corpse_haul_journey.py /absolute/path/to/mariadb/server   # in make test-db
python3 tests/async/test_corpse_trap_restart_journey.py   # flat-file; in make test
python3 tests/async/test_corpse_trap_restart_journey.py /absolute/path/to/mariadb/server mariadb   # in make test-db
```

`test_corpse_haul.py` runs the production selection, pickup and reporting under
ASan/UBSan: NPC and player presentation, coin-only and mixed loot, scrap, a later pile
with nothing to take, the count cap, a malformed sibling cycle, failed live delivery and
strict NPC publication. Its extraction fixture releases PC data, clears the descriptor
and live runtime identity, and moves the actor to NOWHERE. Lethal, sleep, teleport,
removed/moved source and sibling cases cover first/middle/last/only traps, filtered and
unfiltered corpse/floor selection, surviving traps, cancellation after insertion,
allocation failure, and another get by a new runtime identity with the same PID.
Finalizers and combat remain stubbed here; full command/death coverage requires the
server journey. `test_get_trap_lifecycle.py` compiles the production `get()` and
finalizers with actual character/object structures. It verifies that lethal, teleport
and sleep traps stop container and floor pickup before delivery, logs, waits or lighting;
that surviving traps reject their item; and that ordinary/ioun, crumble-on-loot, full,
partial and capped coin, refusal and NPC paths report only actual delivery. Its lethal
fixture releases PC data and removes the actor's live runtime identity under ASan/UBSan.
The MariaDB journey uses three real accounts: the looter kills
Raoul in combat, a second player at the corpse sees the haul start, and a third in the
next room sees nothing. After a real player death it repeats a mixed equipment-and-coin
haul on the player's corpse. Each stage asserts the haul, observer output, and the saved
custody and wallet; a reconnect keeps the inventory without replaying the haul.

The trap restart journey creates a player corpse whose ordinary and ioun items precede
a lethal object-triggered room-fire trap. It runs the real `get all corpse`, checks that
the actor and a same-room mortal bystander receive the room effect and return to their
account menus while a durable same-room NPC, a remote staff client and the server remain
live, and reconnects the actor to prove the abandoned batch does not replay. It then
restarts the server and verifies that the original corpse retains the same item UIDs and
the trap's consumed charge on MariaDB and flat-file authority. A final direct pickup
proves the spent trap did not rearm. The journey uses only synthetic accounts and a
private world; the MariaDB leg is registered in `make test-db`.

## Falling: Safe Fall, Climb and breakable floors

- A successful Safe Fall halves the computed impact damage, rounding down; the pre-skill
  minimum of two gives a successful minimum of one. The strict skill comparison, water
  landings, flying or levitating characters, mount and rider handling and breakable-floor
  behavior are unchanged. (The old left shift doubled the damage.)
- Climb needs the active Climb affect and catches an initiating fall with
  `clamp(effective_skill, 0, 100) / 2` percent probability, rounded down: at most 50%,
  whatever the skill bonus. The effective skill still includes the Mental Anguish
  restriction. Climb does not recheck once a falling event is under way.
- A lethal impact on a breakable floor ends there: no dispel, no further fall, and the
  message reports the impact without saying the floor shattered. Death owns corpse
  placement, and the dead actor does not destroy the floor. A survivor still dispels it
  and keeps falling.
- A fall in progress refuses every command but petition and return ("You are falling!")
  until it lands, the faller is told "You tumble downward!" on every step, and a step
  fires only in the room it was scheduled in: a faller summoned or teleported between two
  steps falls again from where they are if that is open air, and stands if it is a floor.
  (A move typed inside the gap between two steps used to run, and the fall then landed
  wherever the walk went, three rooms away in the report; the first gated build left a
  faller moved into open air hovering there.)
- A down exit back onto the room itself, or onto the room the step came from, lands the
  fall instead of continuing it, and `test_falling_world_exits.py` refuses any longer loop
  of down exits in `areas/wld`. (Three live rooms fell forever, and once commands were
  gated, nothing but a god or a reboot ended it. The two chasm rooms of the northern
  wilderness now fall down the chasm; the Pocket of Exile lands on its disc.)
- The flat 80 to 120 impact term grows with the fall: a third of it for a one-room fall
  (speed 31), all of it from speed 90. The hit-point term, the agility deduction, the
  minimum of 2 and Safe Fall are unchanged. (Flat, it was most of a low-level
  character's hit points for a single room.)

```sh
python3 tests/async/test_falling_skills.py
python3 tests/async/test_falling_world_exits.py
python3 tests/async/run_falling_skills_journey.py /absolute/path/dms_flat
python3 tests/async/test_lethal_floor.py
python3 tests/async/run_lethal_floor_journey.py /absolute/path/dms_flat nonlethal
python3 tests/async/run_lethal_floor_journey.py /absolute/path/dms_flat lethal
```

`test_falling_policy.py` pins the arithmetic, the scaled impact term at speeds 31, 43, 90
and 250 included. `test_falling_skills.py` runs the production `falling_char` under
ASan/UBSan and exhausts every 1-100 roll for negative, zero, boundary, ordinary and
above-cap skill values: success and failure, short and long falls, odd-damage rounding,
minimum damage, a lethal
threshold, water, a breakable floor, mount and rider, flight and levitation, Climb active
and absent, Mental Anguish, initial against already-scheduled falls, a step whose
faller has left the scheduled room, and down exits that loop. Its journey walks a character off a ledge, lands,
saves and reloads in the landing room, then steps onto a shelf with a certain fall chance,
types a move behind the command that starts the fall, and sees it refused and the landing
in the fall's own room. The fixture is a
Thief so login keeps Safe Fall; skill 1 always fails the strict comparison and skill 100
can fail on rolls 100 and 101, so failed rolls retry up to five times, and increased
damage fails at once.

`test_lethal_floor.py` runs the production falling function under ASan/UBSan with lethal
and nonlethal PC, NPC and mounted impacts: no dispel or event after a lethal one, and
damage, one dispel and one continuation after a living one. In the journey a staff player
casts a real wall of ice across the shaft and the other falls three steps to pass speed
43. `nonlethal` checks the impact, the dispel, the wall's removal, the landing and the
save. `lethal` checks the death, the intact floor, the corpse in the impact room, the
account-menu release, re-entry, corpse looting (11 of 27 starting items under normal carry
limits, by exact identity), save, restart, and the kept item IDs and death count. Minimal
mode skips corpse restoration at startup, so the journey loots before the restart.

Not covered: the mount, rider, lethal-threshold and floor cases run through controlled
callbacks, not a full-server NPC, mount and rider destruction under sanitizers.

## Generated NPC identity across recovery

Vnums 1255 and 1256 are shared templates. Each instance owns its generated strings, race,
class, level, size, base statistics, combat bases, damage dice, spell slots, affects,
aggression, act flags and four coin balances, and reloading only the template lost them.
The bounded `GNP1` extension carries those fields in the portable state codec, without
pet ownership or summon timers.

- File copyover version 14 appends the extension after each NPC's inventory. The fixed
  record layout is unchanged and readers still accept versions 12 and 13.
- The Redis record codec accepts an optional validated extension after its affects and
  transport data; ordinary wire records stay byte-compatible.
- Recovery applies owned strings and base attributes before affects and equipment, then
  updates derived values and reinstates the saved resource values.
- File copyover keeps all four coin balances. Redis world capture leaves currency out, and
  its extension zeroes all four, so neither a stale replay nor a random template wallet
  comes back.
- An old snapshot holds nothing to recover the identity from. It stays readable, keeps its
  instance, and logs a recovery review with vnum, instance and room. A degraded, unowned
  template is written as an empty extension so it cannot block saving the world. No name
  is invented and no shared prototype string is freed.

```sh
python3 tests/async/test_generated_npc_state.py
python3 tests/async/test_world_recovery_codec.py
python3 tests/async/test_world_recovery_pipeline.py
python3 tests/async/test_world_singletons.py
python3 tests/async/test_copyover_custody.py
python3 tests/async/test_redis_fault_recovery_live.py
python3 tests/async/test_redis_floor_store_live.py
python3 tests/async/test_generated_npc_journey.py <server> file
python3 tests/async/test_generated_npc_journey.py <mariadb-server> redis
```

`test_generated_npc_state.py` runs the production capture and apply, the file extension
helpers and the Redis wire codec under ASan/UBSan for both vnums over five cycles: exact
encoded state, string ownership, an ordinary control, legacy handling, truncation,
oversized data and vnum mismatch. The `file` journey sets strings through real staff
commands and runs two live file copyovers with stable IDs, counts and base stats. The
`redis` journey uses a private Redis and a MariaDB schema: an acknowledged snapshot, a
forced crash, a clean reboot and the kept identity. Redis needs a SQL season epoch, so a
flat-file server cannot run it.

Not covered: the full random-world generator (the journeys use controlled instances of
1255 and 1256), and generated equipment across a file copyover, which still stores NPC
equipment by vnum.

## Item race restrictions and races without a bit

An item's race list (`anti2_flags`) holds one bit for each of races 1-32. Races 33-37
(pillithid, kuo-toa, wood elf, firbolg, tiefling) and race 0, which a mob with an unknown
race code gets, have none: no deny list names them, and every allow list leaves them out,
as for the races above `RACE_PLAYER_MAX`. `can_char_use_item()` and
`can_prime_class_use_item()` used to shift past the 32-bit word for them, which x86
wraps: a firbolg was judged as a grey elf, refused the items denied to grey elves and
given the ones allowed only to them.

```sh
python3 tests/async/test_item_race_restriction_runtime.py
```

The test compiles both production functions with UBSan stopping at its first report and
checks every race from 0 to 100 against each of the 32 bits, as a deny list and as an
allow list: a race is refused or admitted by its own bit only, and an illithid by none.
Without the fix it stops at race 0, and from race 1 at the pillithid. Not covered: a
journey in which a character of one of these races wears such an item.

## Journey clients and an ANSI escape split across reads

The journey clients strip colour escapes from what the server sends. They stripped each
socket read on its own, so an escape that the end of a read cut in two survived: the
generated NPC journey once failed `make test-all` on `Cha:  87[0;1;33m ( 87)` from
`stat mob`. The shared `MudClient` in `test_flatfile_combat_journey.py`, its copy in
`test_account_recovery_journey.py` and the copyover journey's compressed reader now hold
back a cut-off escape until the next read completes it.

```sh
python3 tests/async/test_journey_client_ansi_split.py
```

The test feeds both clients an escape split across three reads through a socket pair and
fails without the fix. Not covered: the copyover journey's own reader, which shares the
method but is only exercised by its journey.

## Kick messages for every race

`kick_messages()` picks message 0 to 19 by the victim's race in nine arrays, and the
victim's hit messages had 19: the insect line was missing, so each later race read the
next race's message and the default race read past the array. A missing comma glued each
array's last message to an unused `""`, which hid the count. clang-tidy's
`bugprone-suspicious-missing-comma` found it. The line is back, the `""` entries are gone,
and a `static_assert` in `src/combat/kick.c` holds all nine at 20; it fails to compile on
the old arrays. Not covered: whether each message suits its race.

## Launcher: a service stop is recorded

systemd stopped the launcher with the server (`KillMode=control-group`), so the launcher
never wrote its `server_reboots` row: one restart in ten was recorded. The unit sends
SIGTERM to the launcher alone (`KillMode=mixed`); the launcher runs the server as a
child, passes the signal on, waits for the shutdown, writes the row and exits without
starting the server again or pausing ten seconds. The issuer and reason go in as hex, so
a reason with an apostrophe is recorded, and a failed insert is reported. A stop that lands
between the server's fork and `SERVER_PID=$!` still reaches the new server; the trap sent
it to the last run's PID.

```sh
tests/async/with_disposable_mariadb.sh python3 tests/async/run_launcher_stop_journey.py
python3 tests/async/test_flatfile_launcher.py
```

The journey runs the real launcher with a stand-in server on a disposable MariaDB and
sends SIGTERM to the launcher alone, twice. The launcher test runs the launch block with
the launcher signalling itself inside that window. Neither runs systemd.

## Line editor: every line freed

`FREE()` names its argument twice, so `FREE(lines[i++])` freed one line, nulled the next
and skipped two. `edit_free()` and `edit_insert_data()`'s refusal of too many lines leaked
every second line, an odd count read past the terminator, and `edit_free()` never freed
the line array itself (32 KB per edit). Both loops free `lines[i]` and step once, and
`edit_free()` frees the array. clang-tidy's `bugprone-macro-repeated-side-effects` found it.
No player reaches this editor today: nothing outside `src/misc/editor.c` calls
`edit_start()`, and `close_socket()` never touches `d->editor`, so the leak never ran in
the game. Wiring the editor back in also needs `close_socket()` to `edit_free()` it, or a
disconnect in the middle of an edit leaks it.

```sh
python3 tests/async/test_editor_free.py
```

The harness runs the production editor under LeakSanitizer: three lines started and freed,
and three lines added to an editor that holds two, which it refuses. It fails without the
fix, on either loop alone.

## Log lines a reader can use

A zone command that does not load (`M`, `F`, `R` whose chance roll misses) is logged with
its mob and room vnums, not the boot's internal indices. A connection reset before the
server accepts it has the host `unknown`, not a color code that went into every line about
it. A shutdown, reboot or copyover writes its kind, issuer and reason to the status log
instead of the players' broadcast with its color codes and line ends. The issuer's wiz row
is written when it is scheduled; the completion runs with no character, so its wiz row was
never written, before or after this change.

```sh
python3 tests/async/test_log_hygiene_journey.py
```

An `R` that misses its roll after an `M` that loaded the rider no longer goes on with a
NULL mount, which crashed the zone pass.

The journey boots a flat-file server with zero-chance `M`, `F` and `R` commands and a
zero-chance `R` after a loaded rider, resets a connection before the server accepts it,
and stops the server with SIGTERM.

## Maintenance scheduler state file

The state file defaults to `runtime/maintenance-scheduler.state`, outside the `bin/` tree
that `make clean-all` removes. The scheduler makes a missing directory above the file
with mode `0700`, and a write that fails is logged once per failure streak while the
worker retries every second. The old default under `bin/server/` vanished under a running
server during a clean rebuild, and nothing said so.

```sh
python3 tests/async/test_maintenance_scheduler.py
```

The harness drives the production scheduler with a stubbed job: a state path under one
missing directory level is made and written, and one under two levels stays unwritable and
is retried after a pause.

Not covered: a configured `MAINTENANCE_STATE_FILE` that still points under `bin/`; that is
the host's configuration.

## New databases on MariaDB 11.8

The sealed verifiers of migrations 0031 and 0032 accept only MariaDB 10.11 and MySQL 8.0,
so a database built on MariaDB 11.8 stopped at 0031. Their files cannot change (every
history holds their checksums), so the manifest lists an 11.8 verifier for each, which the
runner runs in their place on 11.8; the history keeps the sealed checksums. On 11.8 their
metadata fingerprints are 10.11's.

```sh
python3 tests/async/run_migration_runner_engines.py
RUNTIME_DB_IMAGE=mariadb:11.8 tests/async/run_runtime_compatibility_mysql.sh
python3 tests/async/test_immutable_migration_runner.py
```

The first builds a database through the runner on MariaDB 11.8, 10.11 and MySQL 8.0, checks
one history checksum on all three, and that an edited history row stops the runner. The
second applies every step and verifier on 11.8, as the runner chooses them, and runs the
boot check's drift rejections. The unit test covers the manifest's checks of the 11.8
verifiers. No test covers an engine besides these three.

## Ownership records of items that stopped existing

A save never releases an `item_current_owner` row, so a player's row for an item that was
dropped and then extracted (or eaten, decayed, dissolved) was counted on every login,
forever, and the table grew with play. Each boot now deletes a player's active rows whose
item no stored payload carries, in both backends, before the writer starts: no
character's or pet's items, no locker, corpse or saved room item, and on flat-file no
delivery the record's player still has pending from a committed purchase or grant. A row
whose item an older copy still carries stays, since a load skips a copy whose row names
someone else: after a crash that follows a hand-over, the row is what keeps the giver's
copy out (found in the change's review); and a flat-file purchase or grant committed
after the buyer's last save is delivered by the next login only while its record names the
buyer (finding 2). A row an auction's custody row, an `artifact_domain_state` row or a child
row still references is kept, and a container goes after its contents. A flat-file player
file or store the reap cannot read stops it with nothing deleted. Flat-file world recovery
counts a player's record as an owner only while the player's next load holds the item, as
MariaDB's does with `player_items`, so a floor copy whose record only keeps an older copy
out is restored rather than lost. The per-login `missing_payload_rows` count and the
per-save `unowned_object` line are traces (`DURIS_PERSISTENCE_TRACE`). Landed in `a55ccef17`.

```sh
tests/async/run_player_save_claim_mysql.sh
python3 tests/async/test_player_save_claim.py
python3 tests/async/test_player_snapshot_capture.py
python3 tests/async/test_boot_log_hygiene.py
tests/async/with_disposable_mariadb.sh python3 tests/async/run_world_restart_journey.py /absolute/path/dms_new taken
tests/async/with_disposable_mariadb.sh python3 tests/async/run_world_restart_journey.py /absolute/path/dms_new handover
tests/async/with_disposable_mariadb.sh python3 tests/async/test_mysql_combat_journey.py --server /absolute/path/dms_new
python3 tests/async/test_flatfile_combat_journey.py
```

The two claim harnesses hold the rules on fixture rows: a stale leaf and a stale container
with its stale contents go; a row with a payload row, a legacy pet's, a quarantined one,
another owner's, one whose item an older copy carries (another player's payload or pet, a
locker, a corpse, a saved room item) and (MariaDB) one an auction's or an artifact's row
references or whose contents a payload still holds stay, and the older-copy rows go once
the copies are gone. The flat-file harness also commits a creation grant after the
player's last save: the reap keeps its record and the login delivers it, and once a save
has carried it and the item is used up the next reap takes the record; the newcomer whose
file holds a handed-over copy loads without it; an unreadable player file stops the reap;
and world recovery counts as owned what a load holds and not a record that only keeps an
older copy out. The `taken` scenario is the real path without the switch: get a
zone-loaded mace, save, drop it, save, crash, boot; the mace's row and the dissolved
starter kit's are gone, every remaining player row has a payload row, the login counts
nothing and the saves wrote no `unowned_object` line. The `handover` scenario is finding
1's crash: the banana's row survives the boot, the giver loads without its older copy
(`load_skipped` in `logs/log/dupes`), the floor copy is restored, and after both save one
character holds it. The combat journeys restart with a ghost record under the banana and
the switch on: each save of the looted banana wrote an `unowned_object` line, and after
the boot the ghost is gone, the rows whose items the save holds stay, and the login counts
nothing. The capture test runs the production save with the switch off (three saves, no
line) and on (one line per save).

Not covered: a stale container in the flat-file catalog whose contents a payload still
holds (no transfer path produces that state there; the rule is held by the MariaDB
fixture, where the foreign key is), the flat-file reap and world recovery on a live
server (the harness drives the repository functions), and the reap on a long-lived
database with every owner type populated.

## Plain HTTP on the WebSocket port

A GET without an upgrade, a crawler's or a browser's, was dropped unanswered, and the
tunnel in front of the port turned that into an error page. It gets `426 Upgrade
Required`, and `GET /robots.txt` gets `Disallow: /`; both close. An upgrade still opens.

```sh
python3 tests/async/test_websocket_runtime.py
```

The harness runs the real `websocket_parse_handshake()` over a socket pair. No test sends
these through the tunnel.

## Player save decoding refuses what it cannot read

`player_snapshot_decode()` and `player_item_snapshot_list_decode()` returned the decoder's
result when an element's reader refused a value, and a reader that refused a value without
naming why (a status field out of range, for one) left that result at `ok`: a damaged save
"decoded" without writing the snapshot, and the caller went on with an empty one. The
`player_snapshot` fuzz target found it in its first minutes. A refusal now returns
`invalid_value` unless the reader named another reason.

```sh
python3 tests/async/test_fuzz_corpus.py
```

The crash input and the twelve corpus inputs that failed the same way are in
`tests/fuzz/corpus/player_snapshot/`; the replay fails without the fix.

## Production launcher and a staged development build

`scripts/cycle_mud.sh --production` promotes only a `bin/server/dms_new` stamped
`mariadb/production`. One with another stamp is logged and left where it is, and the
stamped runtime binary runs; the launcher exits only when the runtime binary is unstamped
too. It used to exit on the staged stamp alone, so a development build staged by the
regression suite kept the systemd service in a ten-second restart loop with a valid
`bin/server/dms` beside it.

```sh
python3 tests/async/test_flatfile_launcher.py
python3 tests/async/test_production_service.py
```

The launcher test boots a fake stamped runtime binary past a fake development build in
production mode and checks that the staged file and its stamp are untouched, then removes
the runtime stamp and expects the refusal.

Not covered: a real production boot; the journeys run the server directly.

## Quest EXP line

With the EXP display on (`toggle experience`), a world quest reward prints one `Quest EXP:`
line with the amount the character was credited, after modifiers and caps, and prints none
when nothing was credited. An immortal is credited nothing and gets no line; its staff log
line (`logexp()`, "would have gained") keeps the award it would have had.

```sh
python3 tests/async/test_world_quest_xp_feedback.py
```

The test compiles `gain_exp()`, `display_gain()`, `quest_kill()` and `quest_full_reward()`
from the source on both backends and checks the line for kills, turn-ins, every cap and
exit that credits nothing, and an immortal, whose staff log it reads. No test turns in a
quest on a running server.

## Random drop message for a pet's kill

When a player's pet kills an NPC and wins the equipment roll, the "salvage a piece of
equipment" line goes to the pet's owner. It used to go to the pet, which nobody reads, so
the owner never learned an item was on the corpse. The drop rolls and the item still use
the pet's own level and luck.

```sh
python3 tests/async/test_random_drop_owner_message.py
```

The test is a source contract on the drop block in `die()`. No test kills an NPC with a
pet on a running server.

## Riposte after a participant is removed

Riposte keeps process-local character identities, the original room and height, and the
chosen weapon's slot and UID. Before it continues after an attack it resolves both
characters, requires the same living participants in the original place, and checks the
live equipment slot before it touches the weapon, so reused character or object storage
cannot become a follow-up target. The innate second-hand strike checks its captured
secondary weapon. `hit` rejects dead participants before its first skill read.

```sh
python3 tests/async/test_riposte_lifetime.py
```

It runs the complete production `try_riposte` under ASan/UBSan with controlled attack
callbacks. Ordinary, expert, elite, innate, berserker and follow-up branches keep their
living attack counts. The destructive cases follow each possible hit: death with cleared
player storage, extraction, runtime identity reuse, room and height changes, weapon
removal and weapon identity reuse. Follow-up damage returns false after invalidating
participants, so the return value is not read as survival. The test also runs `hit`
through its initial guards only.

Not covered: real reflective damage and a proc-driven extraction during an expert or elite
riposte on a full sanitizer server.

## Setbit flag bits beyond a 32-bit field

`setbit` sets a flag bit only where its 32-bit field has one, and otherwise answers
"That field has no bit for that value." An item's race list offers every race, but only
races 1-32 have a bit: `setbit obj <item> race firbolg 1` asked for bit 35, which x86 wraps
to bit 3, so the item denied grey elves instead. A flag given by number outside 0-31 did the
same.

```sh
python3 tests/async/test_setbit_flag_bits.py
```

The test compiles the production `setbit_parseTable()` and `ac_bitCopy()` with UBSan
stopping at its first report, against an item race list built as `setbit_obj()` builds it
and a numbered flag field: races 1 and 32 set and clear their own bits, race 36 and the
numbers -1, 32 and 40 are refused and change nothing, and bit 31 is set. Without the fix it
stops at race 36's shift. Not covered: the other `setbit` field types.

## Shop rates within the loader's bounds

The loader caps a shop's buy rate at 0.8 and logs `Old buy/sell` when it changes one. Nine
shops (31310, 47061, 47070, 47097, 47116, 59081, 59097 at 1.0, 89091 and 89118 at 0.9)
asked for more, so every boot logged nine lines. Their files now ask for 0.8.

```sh
python3 tests/async/test_flatfile_full_world_boot.py
```

The full-world boot fails on any `Old buy/sell` line in its debug log.

## Shopkeeper scans at boot

Most of every boot was one scan. Each zone reset of a fixed shop asks whether its keeper is
already alive, which walked the character list calling `singleton_shop_id()`, and that
compared every shop's keeper for any mob not bound to a shop: 7.4 billion comparisons a
boot. The SQL restore then walked the list twice for each restored keeper. Now the
prototypes that keep a shop are indexed once, `singleton_shop_id()` returns at once for
any other mob, and `live_shopkeepers()` reads a set of the live NPCs of those prototypes,
kept by `read_mobile()` and `extract_char()`; the reset check and both restores use it.

```sh
python3 tests/async/test_world_singletons.py
```

The harness asks `singleton_shop_id()` about a bound keeper, an unbound keeper away from
home and in its shop's room, a roaming keeper, a controlled copy and a mob whose prototype
keeps no shop, and checks that `live_shopkeepers()` names exactly what a walk with
`singleton_shop_id()` names, for every shop. Its `read_mobile()` reports a mob before it
marks it an NPC, as the real one does. No test times a boot.

## Snoop notices, audits and recall

ADR 0003: a snoop tells its target when it starts and at every end, at every level that
can snoop, where before the notice was given only below level 58, so no target was ever
told. Every start and end is a `wiz` audit row, at 61 and 62 too. A silent snoop is level
62 only and needs a reason, which its row keeps. A snoop ends the same way on a stop, a move
to another target, a quit, the snooper's link closing and either side leaving the game;
`extract_char()` left an immortal snooper's entry in its target's list. A god switched into
a mob snoops as itself: its stop used to unlink the mob, so the snoop went on while its
target was told it had ended, and the entry outlived the god. `who <name>` shows a silent
snooper only to level 62; a snooped 61 could read it there. A target whose snoops end while
it stays, at a shapechange's return or a switch, is told as at a stop and the row names the
player; before, it heard nothing and the row named the shapechanged body. The channel spell's shared
sight is neither told nor audited. `recall <n> <player>` by an immortal answers
"Disabled by Zusuk October 9 2026".

```sh
python3 tests/async/test_snoop_and_recall.py
```

The harness runs the real `do_snoop()`, its stop helpers, `rem_char_from_snoopby_list()`,
`who`'s `list_snoopers()` and `do_recall()` under ASan and UBSan, with the lookup and output
stubbed. No test snoops on a running server.

## Staff title with a mob's name in it

Staff type `title <player> <text>` to title another player. Any other first word, a mob's
name included, starts their own title, as `title <text>` does for mortals. A level 62
typing `title can be reached on Discord` beside a mob called "can hy trader" used to
title the mob: the server printed "Title Bestowed" with the mob's keywords, then refused
with "That field is undefined for monsters", and the staff title stayed blank.

```sh
python3 tests/async/test_staff_title.py
```

The test compiles the real `do_title()` under ASan/UBSan with a mob, two staff and a
mortal: a mob's name, another player, oneself, a bare `title`, a superior and a mortal's
first word. `do_string()`, which stores the title, is a stand-in.

## Studio-proc trigger sources and duplicate records

`make world` builds `areas/world.trg` with `make_trg` (`areas/src/trg/make_trg.c`) from each
area's `areas/trg/<area>.trg`, and `make_trg` fails generation, naming the file and line,
when a source's framing is wrong. It writes every line with one newline, so a source
without a final newline cannot run into the next one; inside a trigger, a line starting
`S`, `T ` or `#` means its `~` is missing. It starts its output with a marker line and
refuses to replace an `areas/world.trg` without it that holds a record, which was the
hand-written source before `make_trg`. `scripts/cycle_mud.sh` refuses to boot when
generation fails, and `make world` regenerates after a source is removed. At boot,
`studioproc_boot()` logs and skips a second record for a target that already has one: only
one record per target dispatches, and a second bind lost the target's own C proc.

```sh
python3 tests/async/test_make_trg.py
python3 tests/async/test_studioproc_duplicate_record.py   # builds or reuses a flat-file server
python3 tests/async/test_flatfile_launcher.py
python3 tests/async/test_root_test_harness.py
```

`test_make_trg.py` compiles the tool and runs it on good, concatenated, missing and
malformed sources and on a hand-written `world.trg`. The duplicate test boots the
flat-file server on two records for room 22800 and reads the status log.

## The tick's work spread over its pulses

On an idle world the tick was one slow pulse and three busy ones: `affect_update()` walked
all 56,000 mobs at once (40 to 140 ms on staging), the regeneration events it started all
ran 10 pulses later, each rescheduled once more after filling its mob only to find it full,
and `generic_char_event()` ran on the same pulses. A regeneration event that fills its
character no longer reschedules; `StartRegen()` gives a mob's first event its own point
within the delay, and the event counts the pulses that passed, so the gain is the same;
`generic_char_event()` runs five pulses off the 20-pulse grid; and mobs count their affects
down in 20 slices, one every 15 pulses, while players keep the tick's first pulse.

```sh
python3 tests/async/test_tick_work_spread.py
```

The harnesses run the real regeneration events, `StartRegen()` and `affect_update()` over
a whole tick. A mob whose vitality fills does not reschedule, and loses and regains points
as before; each affect, a mob's and a player's, counts down once a tick. A mob's buff now
wears off at its slice's pulse rather than at the tick boundary. No test runs an idle world
for a time; the plan's item 18 records that measurement.

## Telemetry writer: schema check, round trip and the gap record

The SQL telemetry writer (off unless `TELEMETRY_ENABLED` is set) proved at startup only
that four tables existed, so a schema its serializers could not write to was reported
usable until the first affected record failed; no gate wrote a telemetry record to a
database, which is how the repository and the migrations came to disagree on column
names; and a restart or copyover during an SQL outage dropped the queue with no record of
the gap. Now the worker validates every column each record kind writes (name, type,
signedness, width, nullability, default), the InnoDB engine and the replay and projection
indexes against `information_schema`, and probes its SELECT, INSERT and session UPDATE
grants with zero-row statements, before a record is admitted; a refusal opens the circuit
with the cause on the `telemetry_health` line (`failure_class`, `error`, `schema_check`:
table, column, column-type, index or engine) and the game runs on. The worker registers
its producer in `TELEMETRY_OUTAGE_LEDGER_DIR` before it qualifies, samples bounded
counters about once a second and writes a terminal observation when it stops; a later
producer turns an unfinished one into an unknown tail; `scripts/telemetry/outage.py`
exports the ledger. A player who enters, or is recovered by a copyover, while the writer
is still qualifying is retried by the descriptor sweep. Landed in `338092a78`; the writer code is
LuminariMUD's (`e0e837102`, `03da1882d`, `b3fb28b9f`).

The review of that change found three ways the writer stayed off for good after one event, and a
line that did not say why. A pending file that was not a whole frame (what a full disk
or a crash leaves between the create and the rename) refused the ledger on every later
boot; now the writer removes its own staging file when a write fails, an open removes
one that does not decode, and a failed sample no longer ends the worker. At 256
lifetimes the ledger refused the 257th; now it is kept as an archive and the chain
starts again. A transient failure at qualification (the advisory lock held, a lock wait
past the two-second read timeout) was retried eight times inside a quarter of a second
and then opened the circuit for the rest of the process, on master too; now it is
retried at a one-second cap for as long as it lasts. The health line and `world
telemetry` print `storage_check` for a ledger refusal.

```sh
tests/async/with_disposable_mariadb.sh python3 tests/async/test_telemetry_repository.py --sql-fixture
tests/async/with_disposable_mariadb.sh python3 tests/async/run_telemetry_schema_boot_journey.py --server /absolute/path/dms_new [--misnamed-server /absolute/path/dms_misnamed]
python3 tests/async/test_telemetry_transport.py
python3 tests/async/test_telemetry_outage.py
python3 tests/async/test_telemetry_runtime_outage.py
python3 tests/async/test_telemetry_gameplay_adapters.py
```

The repository test applies the whole migration chain to a disposable MariaDB with
`migration_runner.py`, checks the history head, then writes every record kind 1 to 8,
replays each for `duplicate_identical` (the repository reads every mapped column back and
compares) and changes one field for `duplicate_conflict`; with `--sql-fixture` a missing
`TEST_DB_*` setting is an error, so the `telemetry_repository` leg of `make test-db` cannot
report the SQL part as skipped, and a column name the table lacks fails the INSERT. Its
startup cases rename a combat and a progression column, change a type, a width, a
nullability, a digest length, two defaults and three indexes, add a required column and
take grants away, and pin the failure class, the schema check and the SQL error of each.
The transport test pins that the published health carries the repository's cause. The
schema boot journey, the `telemetry_schema_boot` leg, boots a real server with telemetry
on: against the whole chain (healthy; stopped, copied over and killed, and the ledger
shows `clean_drained`, an `unknown_tail` for the copied-over producer and a `running`
watermark that the next producer turns into `unknown_tail`), against a renamed progression
column (the boot gate refuses that schema with `COMPAT-E003` before telemetry runs), as a
writer that may only SELECT (`permanent-permission error=1142`, game running, nothing
admitted) and, given a build whose `telemetry_columns.inc` names a column the chain lacks,
`permanent-schema error=1054 schema_check=column` with the game running: the production
incident. The outage tests cover the ledger's lifecycle, protection, corruption, a real
SIGKILL and exec, ENOSPC and fsync faults with the retry in place, an empty, short or
torn pending file removed at open, the archive at 256 lifetimes and the export of the
live ledger and of an archive; the runtime journey registration before SQL init, clean
drain, transient recovery, a shutdown with an unresolved commit, a disk-full registration
(refused, no staging file left), a disk-full sample (capture goes on) and a transient
qualification failure retried past the old budget; the transport test the same at the
transport, twenty failed initializations then a healthy writer. The journey adds a boot
with no ledger directory (`storage_check=directory error=22`) and one with a directory
readable by others (`protection error=1`). The adapter test covers delayed qualification,
presence without input and copyover handoffs kept for a later observation.

Not covered: a copyover or kill while queued records are waiting on a real server with
SQL down (the runtime journey simulates the faults; the ledger's content under them is
pinned there), a terminal sample before a copyover's exec (the copied-over producer is an
unknown tail by design today), and MySQL 8 (the gate runs on the wrapper's MariaDB).

## Zone purge with followers

`zone_purge` records runtime identities, resolves each again in its original room, and
checks NPC and morph eligibility right before extraction. Saving `next_in_room` first was
wrong: `die_follower` can extract the next NPC, a summoned follower, during the call.

```sh
python3 tests/async/test_zone_purge_lifetime.py
python3 tests/async/run_zone_purge_journey.py /absolute/path/dms_flat
```

The test compiles the production `zone_purge` and `die_follower` under ASan/UBSan. Its
extraction adapter covers retained and immediately freed allocations, recursive follower
teardown, a moved NPC, reused storage, surviving PCs and morphs, and the object-purge
policy. Combat-reference cleanup belongs to the adapter, so the test says nothing about
production `stop_fighting`. The journey runs 15 cycles of a mortal fighting Raoul, an
immortal purging the opponent, the combat reference clearing, a full zone reset and
another player command, then a save and reconnect across a restart.

Not covered: the original report, an intermittent full-world crash in room 402003 after
purging an apprentice and running `zresetfull`, was never attributed to this defect by a
core or sanitizer trace. A full-world sanitizer journey with a real summoned
master-and-follower chain would settle it.

## Connection timeout observer cleanup

The LR-11 churn fixture used to close incomplete TLS/WebSocket sockets before
joining their timeout observers. A fast churn loop could therefore count its own
cleanup as a server expiry or race the receiver. The fixture now retains and
joins both observers through their existing 45-second deadlines, asserts both
results, then closes the sockets in `finally`. No server code changes.

A controlled before/after probe executes the actual `connection_churn` function:
the former version closes sockets before observations finish; the repaired version
waits for delayed server expiries, and an unexpired handshake fails before cleanup.
The exact repaired test bytes also pass the full isolated server journey via:

```sh
PYTHONPATH=tests/async DURIS_REGRESSION_BUILD_CACHE=bin/regression-artifacts python3 bin/analysis/integrated/connection-fixed.py
```

The outside-tree copy kept the qualification tree stable and is byte-identical to
`tests/async/test_connection_limit_journey.py`. The real run observed 2,001 attempts,
TLS/WebSocket expiry at 37.53/27.25 seconds, successful reconnect, peak RSS +372 KiB,
33 peak descriptors against 20 initially (10 after cleanup), 264,876 log bytes,
maximum menu response 193.81 ms and no 250 ms pulse overrun. Evidence is retained
in `bin/analysis/integrated/observer-probe.log` and `connection-fixed.log`.
The controlled scheduling probe supplies the regression's before/after evidence;
a normal timely server can pass the old fixture, so its green result alone does
not establish correct observer cleanup.

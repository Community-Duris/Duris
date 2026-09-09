# Issue #157 — Dynamic bartender world-quest rework

Status: implementation complete; local MariaDB-primary and flatfile-primary runtime proofs pass on `codex/issue-157-dynamic-bar-quests`.

Issue: https://github.com/Community-Duris/Duris/issues/157

This document is the repository-owned spec/acceptance contract for the rework. The linked URL supplied during planning was `/pull/157`, but GitHub resolves number 157 as the open issue above; no PR 157 exists.

## Problem

`ask <bartender> quest` currently calls `createQuest()`, which calls `getQuestZoneList()`. The existing path treats the database `zones.quest_zone` bit as the complete zone catalog. The current configured database has all 351 rows at zero, and the flat-file build has no database implementation of `get_zone_info()`, so both paths can produce an empty candidate list before target selection. The caller refunds the fee after a generic failure.

The existing path also performs broad mobile/object scans and temporary prototype loads while a player is requesting a quest. A quest command must not cause a large or repeatable scan on the game thread.

## Goals

- Build a bartender-independent, content-derived global quest catalog at boot.
- Make DB and flat-file builds use the same quest eligibility and weighting policy.
- Treat policy controls as deny-by-exception: all otherwise valid content is automatic; explicit denies can disable named zones and existing unsafe item/target flags, but there is no positive approval list.
- Make zone likelihood vary by player quest level using zone average level, post-floor average item ivalue, and post-floor eligible-item count.
- Give item-rich zones such as Alatorin a strong likelihood while preserving level fit and reward quality.
- Remove general junk objects before reward counts/means are calculated: retain an item exactly when `2 * itemvalue >= accepted_quest_level`.
- Populate the catalog once during boot and make bartender requests read cached data only.
- Preserve the existing world-quest persistence history behavior in both MariaDB and flatfile-primary modes.
- Produce distinct staff/player diagnostics for no eligible zone versus no eligible target.
- Prove a real bartender grants a quest in local development using both persistence backends.

## Non-goals

- No production deployment, migration, or production database mutation.
- No conversion of generated analysis reports into an approval table.
- No replacement of player-specific aggression, visibility, reachability, or completion rules with static assumptions.
- No change to quest fees, daily allowance, experience, epic, sharing, or completion reward policy except where required to select a valid cached item pool.
- No per-request database lookup for zone eligibility or reward statistics.

## Functional requirements

### FR-001 — Global dynamic catalog

For every live player request, the catalog is independent of bartender identity, bartender room, current player zone, continent exits, and source-location map exits. A valid quest destination is selected from the global dynamic catalog.

### FR-002 — Boot-time cache

After the world, mobile, object, special-procedure, map, and index data are loaded, the server builds an in-memory catalog once. The catalog contains:

- every normal zone's content-derived average mobile level;
- runtime-truncated average level for the existing level-window semantics;
- map-room presence used by the zero-map-entry gate;
- cached mobile prototype metadata needed for level, availability, speakability, and pre-31 visibility checks;
- source-eligible reward object VNUMs and their `itemvalue()`;
- post-floor reward VNUM pools, counts, sums, and means for every supported quest level;
- the per-level zone raw likelihood score.

A request may lazily fail closed if a test calls the API before boot, but normal server boot must populate the cache. Repeated quest, map, and abandon commands must not rebuild it.

### FR-003 — Zone eligibility

A zone is policy-eligible only when all of the following hold:

- normal non-sentinel zone number (`0 <= number < 4000`);
- not an explicit deny (`#0` Heavens, `#292` Ailvio, `#536` The Great Realm of Duris);
- not a runtime hometown/town zone;
- content-derived average mobile level exists;
- integer average is strictly inside `(player_level - 7, player_level + 5)`;
- a zero-map-entry zone is rejected below player level 41 and may be considered at level 41+;
- the post-floor local reward pool has at least one item for the accepted quest level;
- target selection later finds a player-valid target.

The catalog must not consult `zones.quest_zone` as an allow-list. Existing DB rows remain compatible and may still be used by unrelated systems.

### FR-004 — Stable content profile

The average level used for policy is derived from loaded mobile prototypes in the zone's room-VNUM interval, not only from the currently instantiated NPC list. Temporary prototype loads used to populate the cache must restore `mob_index[].number` and not remain in `character_list`.

### FR-005 — Per-level likelihood

For player quest level `L`, use post-floor reward statistics and the exact content-derived average level:

```text
level_fit = exp(-abs(zone_average_level - L) / 6.0)
value_factor = post_floor_average_ivalue / L
item_count_factor = post_floor_eligible_item_count
raw_score = level_fit * value_factor * item_count_factor
likelihood = raw_score / sum(raw_score for all positive policy zones at L)
```

The item-count factor is linear by design. This is a probability distribution over positive-score, policy-valid zones at that level. A zero-item/fallback-only zone has raw score and likelihood zero and is not selected when a local reward is required.

### FR-006 — Target selection

- Kill and ask targets use the level band `[player_level - 4, player_level + 5]`.
- Kill targets require at least two currently available instances before the temporary selection probe.
- Ask targets preserve the existing exactly-one-instance condition before a probe.
- Ask targets must be speak-capable and not player-aggressive.
- For players below level 31, ask targets with `AFF_INVISIBLE`, `AFF2_CONCEALMENT`, `AFF3_ECTOPLASMIC_FORM`, or `AFF_HIDE` are rejected.
- Player-specific aggression remains authoritative; cached metadata only narrows static candidates.
- Temporary dynamic aggression probes are bounded and operate only on cached candidates, never by scanning every mobile prototype for every request.
- Completed targets are excluded for the rest of the request, and selection retries distinct targets of the same quest type in the same zone before moving on. A request permits at most 32 target-history checks across all zones and types; a history-read error stops it without assigning quest state.

### FR-007 — Item floor and reward selection

An object is retained in the quest reward pool at accepted quest level `L` exactly when:

```text
2 * itemvalue(object) >= L
```

At level 11, ivalue 1–5 is rejected and ivalue 6+ is retained. The reward selector uses the cached post-floor pool for the quest's accepted level. If that pool is empty or the cache is unavailable, the existing random-equipment fallback remains explicit and logged.

### FR-008 — Failure feedback and fee safety

`createQuest()` reports whether failure occurred because no zone survived policy or because no target could be selected. The bartender refunds the fee in either failure case and gives a distinct player-visible response. Successful creation remains the only path that assigns quest state.

### FR-009 — Persistence parity

MariaDB-primary and flatfile-primary builds both:

- boot successfully with the same world-quest catalog;
- allow the bartender command to grant a quest;
- persist the existing world-quest completion history through their respective `sql_world_quest_*` adapter paths;
- retain daily allowance and duplicate-target checks;
- never require `get_zone_info()` or a populated DB `quest_zone` bit for catalog construction.

## Cache and latency contract

- Catalog construction happens once at boot after `boot_db()`, index generation, special-procedure assignment, and map-coordinate setup.
- No `for (i = 0; i <= top_of_mobt; ++i)` or `for (i = 0; i <= top_of_objt; ++i)` scan is allowed in the repeated bartender request path.
- No SQL query is allowed in `getQuestZoneList()`, zone weighting, target candidate enumeration, or cached reward selection.
- Abandon/map/quest command branches must not trigger cache rebuilds.
- Boot emits a bounded status line with zone/mobile/reward counts and elapsed cache-build duration, without secrets or per-item spam.
- Temporary loader probes must not alter live mobile/object counts or leave world entities behind.

## Acceptance matrix

| ID | Evidence | Acceptance |
|---|---|---|
| A1 | Pure policy math harness | Item floor boundaries 5/6 at L11, 9/10 at L20, 20/21 at L41; likelihood formula and zero-pool behavior pass. |
| A2 | Source contract | Explicit deny controls contain 0/292/536; no runtime path requires DB `quest_zone=1`; boot invokes catalog before game loop. |
| A3 | Cache lifecycle harness/source review | Repeated list/score/reward calls reuse the same boot cache; no broad mobile/object scan is present in the command path. |
| A4 | Focused C++/Python regression | Target level band, exact pre-probe availability, hidden/invisible ask gate, and distinct failure reasons are covered. |
| A5 | MariaDB local runtime | **PASS:** `DURIS_WORLD_QUEST_BACKEND=mariadb python3 tests/async/run_world_quest_dual_backend.py` against a disposable MariaDB authority. The branch-built server reported `World quest catalog ready: zones=351 mobile_profiles=17174 reward_profiles=8763 mapless_zones=131`; Mathorn’s Bar room `16633` granted and re-granted a quest; saved `player_data` asserted active state, giver `16553`, target, level 56, start time, and non-denied zone; `zones` readback reported 350 rows and 0 `quest_zone` rows enabled; server exited normally. |
| A6 | Flatfile local runtime | **PASS:** `DURIS_WORLD_QUEST_BACKEND=flatfile python3 tests/async/run_world_quest_dual_backend.py` against a disposable flatfile state root. The client-free branch-built server booted the catalog, granted and re-granted a real Mathorn’s Bar quest, persisted through the flatfile save path, and exited normally; the journey asserted the exact running executable path and recorded `flatfile_state_root_created=true`. |
| A7 | Backend/build gates | `make -C src PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=development`, flatfile equivalent, focused tests, `git diff --check`, format check, and relevant repository tests pass. |
| A8 | Provenance | **PASS:** the runtime harness compares `/proc/<pid>/exe` with the exact branch-built binary, records its path and SHA-256, records the server PID, and uses a disposable run root; it does not use the unrelated August local image. |

## Implementation slices

1. Pure math/policy contract and tests.
2. Boot-time catalog module and Makefile/header integration.
3. Dynamic zone list, weighted selection, cached mobile target selection, and cached reward selection.
4. Failure-reason feedback and caller integration.
5. Dual-backend real runtime journey and evidence capture.
6. Review, format/build/test gates, commit, push, and draft PR linked to Issue #157.

## Rollback

The branch is isolated. Before runtime deployment, rollback is deleting or reverting the branch commit(s). The runtime default remains unchanged until a human merges the draft PR. No production database flag or migration is required for rollback.

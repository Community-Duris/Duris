# Universal zone story integration plan

**Status: per-area mapping foundation implemented on
`codex/discovered-zone-dailies`; deeper journey integration is incremental.**

Maintain this plan as zones are reviewed. Keep static contract classification,
historical objective coverage, economic support, and gameplay qualification
distinct. Complete static bindings do not prove that every special is integrated.

The [builder guide](../guides/ZONE_STORY_BUILDING.md) specifies the shipped
sidecar. The [daily contract](../reference/ZONE_STORY_QUEST_DAILY.md) and
[catalog audit](../reference/ZONE_STORY_QUEST_CATALOG.md) describe its use with
native completion receipts and discovery.

## Model and discovery policy

A zone contains discovery, named stories/requests, supporting services, and
lore. A story has a stable identity, branches, stages/objectives, and a proven
terminal outcome. An event proves an action; possession proves only current
inventory. Rewards remain in existing quest/accounting paths.

Builders author meaningful relationships. Mechanical extraction can inventory
exchanges, item producers/consumers, reset sources, and special references. It
cannot prove that every producer is a required predecessor, every keyword is
an achievement, or every custom item creation proves personal recovery.
Candidate links need source review and appropriate gameplay qualification.

## Delivered foundation

- [x] Optional per-area sidecars; strict schema/duplicate/binding/ownership and
  object validation, bounded reads, atomic application, and fail-closed loading.
- [x] Named alternative terminal groups using existing native IDs, timestamps,
  frozen recipients, and persisted receipts.
- [x] Reviewed exclusions and service categories; consistent achievement,
  leaderboard, daily journal/overview/score projection into meaningful units.
- [x] Live carried/exact-equipment checklists and recorded exchange steps.
  Journal views remain read-only and do not infer item provenance.
- [x] Builder binding export and checked-in catalog regeneration.
- [x] Twin Towers classification, belt/access hint, plant/arrow/wand alternatives,
  distinct feathers, clothing material counts, and unsupported-offering warnings.

No player objective history or new persistence schema is introduced in this
phase. Live inventory is deliberately separate from earned accomplishments.

### Foundation verification — October 2, 2026

SQL and flat-file C++20 server builds passed with the maintained warning profile:

```bash
make -C src -j6 CC=g++-12 BIN_ROOT=../bin
make -C src -j6 CC=g++-12 BIN_ROOT=../bin PERSISTENCE_BACKEND=flatfile \
  DMS_BINARY=../bin/server/dms_zone_story_builder_flatfile
```

The following focused regressions passed:

- `python3 tests/async/test_zone_story_quest_story.py`: the actual Twin Towers
  sidecar, native/Python schema rejection, complete/partial coverage, services,
  exact equipment slot, live checks without writes, alternative daily/story
  projection, leaderboards, copying, and retained receipts after restart.
- `python3 tests/async/test_zone_story_quest_production.py`: native bootstrap,
  valid/absent/malformed sidecars, and missing booted object prototypes.
- `python3 tests/async/test_zone_story_quest_arrival.py`: native player inventory
  adapter, carrying versus wearing the belt, and reads without persistence.
- Existing catalog, production snapshot, domain, tracking, repository,
  flat-file state, and daily evidence-report regressions.

`./scripts/format.sh --staged --check` and `git diff --cached --check` passed.
These are executable mapping/adapter and persistence checks. A full live Twin
Towers journey, including custom dialogue and animal source/decay behavior,
remains pending the adapters below. No production operations were performed.

## Implementation backlog

### Dialogue and durable journey objectives

- [ ] Version an event/objective contract with character, season, story,
  stage/attempt, stable event ID/time, and explicit recipient/credit policy.
- [ ] Dispatch accepted native dialogue after a successful response. Map aliases
  to one authored topic; repetitions and greetings do not duplicate milestones.
- [ ] Persist learned topics and stage outcomes with additive guarded migrations
  and equivalent flat-file authority. Reads never write. Recovery is idempotent.
- [ ] Separate optional learned breadcrumbs from real gameplay prerequisites;
  preserve exchanges that do not require prior conversation. Author branch and
  spoiler visibility explicitly.
- [ ] Add reviewed adapters for specials that provide stable semantic events;
  text/source references alone are not objective evidence.

Proof: aliases, failed interactions, repeated dialogue, branch choices, groups,
store failure, reconnect, replay, and cold restart.

### Acquisition provenance and transformations

- [ ] Project committed lifecycle/movement evidence with actor, item UID,
  source kind/identity, custody history, encounter/reset/creation identity,
  and relevant story/attempt identity.
- [ ] Define possession, first receipt, personal source recovery, personal kill,
  and group assistance separately. Item recovery does not automatically prove a kill.
- [ ] Cover gifts, purchases, theft, player drop/pickup, loans, containers,
  death recovery, and legacy objects with unknown origin. Unknown evidence
  cannot earn a personal-sourcing achievement.
- [ ] Emit Twin Towers animal source evidence using the special's actor context;
  link fresh animal retirement to resulting hide/meat UIDs through a confirmed
  tanning exchange, including freshness and decay.
- [ ] Deduplicate qualifying event/UID credit. An initial gift must not prevent
  later personal recovery; custody cycling must not inflate progress.
- [ ] Author which dailies require fresh events within the current day/attempt
  and which delivery quests permit existing supplies.

Proof: direct recovery/gifts, drop laundering, repeated UID, distinct animals,
decay, transformed lineage, groups, unknown-origin objects, commit failure,
and restart recovery.

### Mixed item-and-coin offerings

- [ ] Extend durable offering admission/transactions while retaining exact
  submitted UIDs and payment legs from the original contract.
- [ ] Commit consumption, payment, rewards, and terminal evidence together.
  Missing items, insufficient funds, and conflicts have no partial effects.
- [ ] Freeze recipients/catalog/policy/objective evidence before commit;
  recover the same result after interrupted publication.
- [ ] Reconcile dialogue/count/cost discrepancies as explicit world changes.
  Until reviewed, the journal uses executable requirements.

Proof: duplicate material requirements, denominations, insufficient funds,
interleaved offerings, interrupted ACKs, both stores, and two cold restarts
with unchanged input/reward identities.

### Dynamic presentation

- [ ] Add a canonical structured journal projection shared by ANSI/plain text
  and a versioned GMCP zone-story extension. Bartender `Quest.Status` remains distinct.
- [ ] Add compact tracking, current stage/next action, concrete blocked reasons,
  source-aware hints, branches, and updates after inventory/equipment/decay,
  dialogue, and confirmed outcomes.
- [ ] Derive costs/rewards from executable metadata. Distinguish story/daily
  completion and optional personal/lore achievements. Current counts can
  decrease while earned history persists.

Proof: ANSI off, narrow terminals/pager, long names, client reconnect, absent
GMCP support, current versus historical status, and no writes/rewards on read.

### World-wide mapping and qualification

- [ ] Expand candidate extraction for producers/consumers, reset sources, and
  specials, including evidence locations and reviewed/unreviewed state.
  Never activate a guessed prerequisite.
- [ ] Classify every native response and quest-like special as terminal,
  intermediate, alternative, service, rejection, lore, or unintegrated script.
- [ ] Report coverage separately for static bindings, dialogue, scripts,
  provenance, economic support, and gameplay qualification.
- [ ] Qualify each mapping revision with a focused player journey. Preserve
  historical native receipts while reprojecting reviewed story families.

## Per-zone integration register

Update the row and its evidence when a zone changes. `Complete` means static
contract classification; it does not claim complete objective coverage.

| Area | Mapping revision | Native classification | Journey/event coverage | Economics | Qualification |
| --- | ---: | --- | --- | --- | --- |
| Twin Towers Forest | 1 | Complete: 84 contracts → 10 stories; 40 rejections and 24 supporting services/trades excluded | Live belt/plant/material checks and existing receipts; dialogue/provenance pending | Supported flowers/arrows/sprite; clothing/tanning mixed offerings unavailable with active accounting | Focused mapping/projection/native adapter/build checks; complete journey pending deeper adapters |
| Other active areas | — | Native fallback; optional sidecars integrate incrementally | Existing discovery and terminal receipts | Existing offering limits | Previous daily qualification; full semantic mapping unclaimed |

## Twin Towers evidence and decisions

- `gardener_block` checks belt 13521 at `WEAR_WAIST` on guarded movement.
  The forest reset loads gardener 13504 and equips it. There is no Q belt award.
- Eight plants are alternative Alvinar successes; eight cut-flower returns
  are rejection responses. Gifted valid plants remain acceptable; personal
  garden recovery needs a later explicit policy.
- Either archer accepts the missing arrow. Other arrow purchases, bluejay
  sales, tanning, and already-tanned returns support stories rather than
  supplying separate story achievements.
- Glor-Linda's three wand contracts are alternatives. Her four feather kinds
  are distinct. The disappearing giver can return through the normal reset.
- Seven Marja clothing requests have exact hide counts and named stories.
  Mixed-offering unavailability is shown honestly; fees are described but
  schema 1 does not pretend to record payment objectives.
- Hanson's buck dialogue promises three chops/three gold; the contract gives
  two chops and charges 500 copper. Marja's backpack dialogue says 20 gold;
  the contract charges 1,000 copper (10 gold). Reconcile deliberately.
- Further integration requires animal source witnesses, freshness/decay,
  tanning lineage, learned topics, and reviewed personal/group credit.
  Every raw keyword is not automatically an achievement.

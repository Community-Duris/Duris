# Universal zone story integration plan

**Status: 36 authored journals, accounting-gated player surfaces, starter/town
coverage, encountered-NPC visibility, and first-journey guidance implemented
on `codex/discovered-zone-dailies`.
Deeper objective and provenance integration is incremental.**

The [execution register](ZONE_STORY_ROADMAP_EXECUTION.md) records comprehensive
source dossiers, shipped changes, newly found dependencies, and verification.
Twin Towers, Plains of Life, Ailvio, Braddistock, Breale and Abandoned Elven
Homestead now have complete source
story maps; their active-world journeys remain unqualified. Schema 3 adds
optional preparation. Ailvio's 78 pair recipes project one family-feeding story;
Braddistock's pet rescue is displayed as an intermediate service. Breale preserves
its deliberate drawing riddle and five independent Triad stages. The Homestead
explains actual gate/key/potion routes with optional access and recipe preparation.

Maintain this plan as zones are reviewed. Keep static contract classification,
historical objective coverage, economic support, and gameplay qualification
distinct. Complete static bindings do not prove that every special is integrated.

The [builder guide](../guides/ZONE_STORY_BUILDING.md) specifies the shipped
sidecar. The [daily contract](../reference/ZONE_STORY_QUEST_DAILY.md) and
[catalog audit](../reference/ZONE_STORY_QUEST_CATALOG.md) describe its use with
native completion receipts and discovery.

The [zone priority roadmap](ZONE_STORY_ZONE_PRIORITIES.md) now orders 220
integration candidates, including the scripted Plains of Life tutorial. Its
first 34 entries have reviewed rough progression stories; the remaining queue
is explicitly provisional static triage. The [active inventory](../reference/ZONE_STORY_ZONE_INVENTORY.md)
covers all 350 catalog zones, 221 native-Q areas, and Q-free script/dialogue leads.

## Accounting requirement and delivery sequence

Active economic accounting is a prerequisite for player zone journals,
discovery/encounter events, zone-story achievement surfaces, and new daily
eligibility. Use the verified native `economic_gameplay_authority::active()`
projection, not an environment flag or the mere presence of accounting tables.
`ZONE_STORY_DAILY_ENABLED` is an additional daily switch; it cannot bypass this
requirement. Offline source audits and builder validation remain available.

Catalog/state bootstrap, exact native completion receipt recovery, persistence,
and deleted-character cleanup remain available while accounting is inactive.
They maintain existing authoritative history without publishing journal prompts,
inventing encounters, or admitting new daily credit. A frozen already-committed
receipt retains its original recipients and eligibility through recovery; the
current activation state must not rewrite its terms or cause reward duplication.

Implement and qualify the remaining additions in this order:

0. **Active-world source admission.** Legacy reset item commands deliberately
   stop under active accounting because they lack durable generation identity.
   Qualify committed O/P/G/E and other item-producing resets, including scenery,
   portals, container dependencies, and NPC equipment before a fresh-world pilot.
   Custom grants and transformations need equally explicit publication results.
   Plains of Life currently clears its lesson tag before confirming its sword:
   fix recoverable tag/grant ordering before claiming a completed tutorial.
   Ailvio additionally needs committed map grants and hidden-ingredient
   replacement. Port active forage before claiming its commission has an
   ordinary supply route. Fishing already submits item grants; qualify its
   success/experience after grant outcome rather than replacing that adapter.
   Pet buy/rent deliberately refuse active accounting and stay optional services.
1. **Shared journal projection and flower pilot.** Render the same canonical
   zone/story/objective state in ANSI/plain text and a versioned GMCP extension.
   Use the selected Client journal design: known contacts, stage/checklist,
   live materials, next action, blocked reason, history, daily status. Qualify
   the actual Twin Towers flower journey, belt at waist, valid alternatives,
   gifts, failed access, consumption, and cold reconnect under active accounting.
2. **Accepted dialogue and durable stages.** Add stable objective events and
   alias-aware learned topics. Use the Plains of Life tutorial and a small
   local exchange chain as the first adapters. Reads and repeated greetings
   create no history. Keep conversational knowledge separate from admission.
   Add successful skill outcomes as a distinct adapter: Ailvio's bandage reward
   currently observes a scheduled attempt before revival. A paid/consumed
   bandage proves material use, not successful aid. Bind actor, exact victim,
   room/reset generation and one reward episode. Qualify failed, aborted and
   concurrent helpers with recovery before introducing a completed objective.
3. **Optional preparation and complete story families.** Add conditional
   subrecipes, optional historical steps, explicit all-stage versus any-terminal
   completion, authored branch/reveal rules, and story attempts. A terminal
   `contracts` array currently means any-of; it cannot express a campaign where
   every independent exchange must succeed. Do not force gifted materials to
   replay a local recipe or treat the first promotion as completing knighthood.
   Group only source-proven equivalent alternatives: Ailvio's complete fish-pair
   set shares one giver/outcome, while independent teacher requests with different
   recipients or rewards stay distinct. Record narrated closure separately from
   actual NPC rescue/voice restoration/rat purge; do not turn prose into effects.
4. **Source and transformation evidence.** Build on committed accounting
   lifecycle/custody evidence for personal recovery, distinct sources, tanning,
   freshness, and lineage. Never infer these from possession or text.
   Review property-driven mechanics as well as assigned specials: key-target
   locks, shared spoken passwords and ITEM_TELEPORT command/destination values
   are executable world data. Live access status and successful unlock/arrival
   history need separate projections; never infer admission from a recipe receipt.
   Use distinct source types for fishing, forage, search discovery and creature
   proof. Cross-area item prototypes can have local reset carriers, as Ailvio's
   drow spores do; prototype ownership is not travel or personal provenance.
5. **Mixed offerings and larger pilot.** Commit materials/payment/rewards and
   stage evidence together, then qualify Twin Towers clothing and grove recipes.
6. **Expand through the priority roadmap.** Review one story family at a time,
   qualify an actual player journey, and update semantic coverage independently
   from static contract classification.

Client updates should follow committed events and live inventory/equipment
changes; reconnect sends a fresh projection. Unsupported facts must be marked
untracked or omitted, rather than rendered as an earned/missing achievement.
Future source hints and subrecipes must respect encountered-contact and stage
visibility. An undiscovered area or an unseen NPC must not be revealed by a
client payload that the terminal hides. Plain clients remain fully usable.

Schema 3 adds an optional boolean on checklist steps. It preserves native
admission and receipt identity, labels optional preparation, and keeps it out
of mandatory `Next:` selection. No new persistence schema, historical event,
or client wire format is introduced by this change. Conditional recipes,
all-stage families, and scripted terminals still require separate implementation.

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
- [x] Schema 2 area introductions, orientation, verified NPC command aliases,
  authored conversation topics, and first outstanding `Next:` checklist action.
- [x] Physical identifiable NPC encounters persisted per character and season;
  unseen contacts and their quest rows stay hidden. Discovery, first meetings,
  and newly recorded receipt progress publish exact journal commands after saving.
- [x] All 27 native starter/town areas have sidecars; creation-room and town-flag
  audit, full Q classification, explicit services and missing-item exclusions,
  mansion rescue grouping, and known Ailvio ingredient routes.
- [x] Active accounting gates player journals, discovery/encounter events,
  zone-story surfaces, and new daily eligibility, independently of the daily switch.
- [x] Eight additional journals classify 86 native contracts across Breale,
  Abandoned Elven Homestead, Krimeneha's Mansion, Bastine, Pine Hollow, Quietus,
  Torg, and Vast Hidden Grove. Source inventory and ordered story roadmap are
  checked in; no deeper unsupported objective kinds are claimed.
- [x] Ailvio source-comprehensive families, one feeding story for all 78 native
  fish pairs, optional medicine/note/jar/seal routes, source hints, complete useful
  conversation topics, and explicit scripted/support gaps. Braddistock's pet
  service and supplied-collar route preserve both native receipts.

Live inventory remains separate from earned accomplishments. NPC encounter
history uses domain header `ZSQF|3` and existing SQL/flat-file buckets; old
domain versions load without guessed meetings. No SQL schema migration is
needed. Learned topics, source provenance, and scripted objective history remain
future work. See the [starter/town register](../reference/ZONE_STORY_STARTER_HOMETOWN_COVERAGE.md)
for the current baseline and its qualification limits.

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

## Starter/town guidance verification — October 2, 2026

Both maintained server builds passed with the commands above. The actual
isolated native journey passed:

```bash
python3 tests/async/test_discovered_zone_daily_journey.py \
  bin/server/dms_zone_story_builder_flatfile
```

It verifies first discovery and its exact command, an unseen giver omitted
from the journal, a later physical meeting revealing the giver and `Next:`
checklist, two real item offerings with one daily bonus, saved reward identity,
and a cold reconnect retaining encounters and progress without repeated prompts.

The story, feature, production/bootstrap, production-catalog, arrival, catalog,
tracking, repository, flat-file state, and daily-report regressions passed.
`test_zone_story_quest_story.py` parses every shipped mapping in native C++,
checks all 27 required areas, verifies visibility, and covers both sidecar
versions. Feature tests verify V2-to-V3 delta upgrade and duplicate/corrupt
encounter rejection. Arrival tests verify visibility, remote-view suppression,
save rollback, and restart. The existing 25-character/60-day/6,000-completion
flat-file capacity fixture also passed.

No production operations or migrations were run. These checks qualify the
universal guidance and persistence paths; complete individual world journeys
remain tracked in the area register.

## Implementation backlog

### Accounting gate and expanded journal verification — October 2, 2026

Both maintained C++20 server builds passed after this batch, including the
normal warnings-as-errors profile. This host's existing hiredis TLS directory
was supplied through `LIBRARY_PATH` for linking and `LD_LIBRARY_PATH` for the
native fixture; no dependency was installed and no production operation ran.

The isolated flat-file gameplay command passed:

```bash
python3 tests/async/test_discovered_zone_daily_journey.py \
  bin/server/dms_zone_story_builder_flatfile
```

The fixture now deliberately keeps accounting inactive while setting the daily
flag. It verifies journal refusal, no discovery/meeting/daily prompts, an ordinary
native quest reward, saved item identity, and cold reconnect. The earlier positive
legacy-mode guided journey above is historical qualification; a full journey
under genuinely active accounting remains the first pilot acceptance requirement.

Focused regressions passed: authored story/native projection (all 36 sidecars),
production catalog and generated inventory, native production/bootstrap, arrival
adapter, feature domain, tracking contract, daily evidence report, and all 27
starter/town source coverage. The arrival adapter tests active/inactive admission,
unchanged state on blocked views/arrivals, replay of an already-frozen receipt,
and deletion cleanup while inactive. Mapping regressions protect distinct Bastine
promotions, gifted drider terminal material, Quietus credential alternatives,
Torg's two equivalent chisel makers, and the family's distinct keepsakes.

The complete priority queue was checked against all 219 non-deferred native-Q
areas plus the Plains of Life, with each included exactly once. All new document
source links resolve. Native definitions, source fingerprint, zone registry, and
the original 28 sidecars are unchanged as parsed objects. Format checks for
changed lines and complete touched C/C++ files, plus whitespace checks, passed.

These results qualify the accounting gate and deployable mapping schema, not
every individual world journey or the future dialogue/provenance/client adapters.

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

### Access, puzzle clues and property-driven mechanics

- [ ] Add reviewed candidates for exact key/exit/container targets, current
  lock/secret/blocked flags, spoken magic-door keywords, item teleport command
  and destination, and source NPC/room/reset ownership. Source extraction is a
  lead list; builder approval defines the journal integration and reveal policy.
- [ ] Project current access separately from previous receipt history: supplied
  keys, already-open doors, equivalent routes and access provided by another
  actor must not demand replay of an unrelated recipe. A live key count alone
  cannot prove that a hidden exit was found, unlocked or successfully traversed.
- [ ] Emit accepted reveal/unlock and confirmed arrival only after real effects,
  with actor, exact source/target generation, room/direction, event identity and
  recipient policy. Qualify shared magic doors and ITEM_TELEPORT travel alongside
  bespoke escort/garden gates, without treating every negative key as a password.
- [ ] Author riddle clues and translations with source references and optional
  staged hints. Breale's east/west wand/broom/hat drawings map to real reagents;
  preserve that puzzle rather than labeling its vocabulary a broken contract.
  Reading a clue is a learned objective only after an accepted examination event.
- [ ] Keep narrated effects distinct from actual mutation. Review Breale's
  bracelet escort and promised learned spells; the Homestead's petals/leaf,
  serpent/dragon and elf closure; and the spellcase opening flavor. Builders
  choose corrected prose or explicit supported effects while preserving existing
  rewards and native receipt identities until a deliberate content revision.

Proof: locked versus merely closed/secret exits, exact similarly named keys,
spoken keyword aliases/repetition/silent or failed speech, supplied/open access,
changed reset generations, consumed/gifted potion ingredients, failed travel,
known versus unseen contacts/clues, cross-area destinations, replay/restart,
read-only journal projection, and spoiler/reveal choices for both clients.

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
- [ ] Version optional preparation, conditional subrecipes, explicit all-stage
  campaign completion, story attempts, and branch/reveal policy. Preserve the
  current any-of terminal semantics for existing mappings.

Delivered portion: plain/ANSI journals now show the first outstanding step,
encountered contacts and conversation commands, live material counts, receipt
progress, and post-save first-time prompts. Structured client projection,
source-aware updates, and durable dialogue/stage events remain open.
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
- [x] Detect literal chained special assignments and omit commented-out ones;
  export per-area Q/M, prototype, reset, and special evidence. Computed aliases
  and dynamic code still need manual review.
- [x] Complete source story dossiers for Twin Towers and Plains of Life,
  including all support/rejection/lore interactions and custom dependencies.
- [x] Complete source dossiers for Ailvio, Braddistock, Breale and Abandoned
  Elven Homestead; retain source findings, optional preparation and exact receipts.
- [ ] Complete comprehensive source dossiers for the other 214 roadmap areas.

## Per-zone integration register

Update the row and its evidence when a zone changes. `Complete` means static
contract classification; it does not claim complete objective coverage.

| Area | Mapping revision | Native classification | Journey/event coverage | Economics | Qualification |
| --- | ---: | --- | --- | --- | --- |
| Twin Towers Forest | 3 | Complete: 84 contracts → 10 story/request units plus 12 service units; 40 rejections excluded | Source-comprehensive dossier; all addressable topics, optional live belt check, materials, receipts; dialogue/provenance pending | Fresh reset sources need durable generation; clothing/tanning mixed offerings unavailable | Source/native checks; full active journey remains pending |
| Plains of Life | 2 | Q-free scripted tutorial; no invented native terminal | Source-comprehensive dossier: optional sign aids, accepted topic/tag/sword and stream travel; scripted history pending | Active reset scenery and confirmed sword grant remain blockers | Source/alias checks; active journey remains pending |
| Breale | 2 | Complete: six independent Q exchanges | Source-comprehensive dossier; drawing translations, optional access/history, accepted materials and named reward guidance; all-stage/learned events pending | Active key/potion/reagent/shop resets need generation authority | Source/native checks; active journey and spell/escort content decisions pending |
| Abandoned Elven Homestead | 2 | Complete: two achievement rows and two preparation services | Source-comprehensive dossier; optional egg/access/statue history, distinct key uses, shared speech/teleport and tapestry leads | Active materials/scenery/nested container sources need generation authority; native exchanges retain existing accounting | Source/native checks; active journey, lineage and accepted travel/lore events pending |
| All 27 starter/town areas | 1–2 | Complete static Q classification; detailed counts and evidence in starter/town register | Orientation, encountered people, concrete delivery counts and native receipt checklists; selected multi-step routes | Existing offering limits; reviewed crafting and equipment services do not create story/daily units | All maps pass source/native validation; isolated native guided journey; individual full-world routes pending |
| Eight additional quest areas | 1–2 | Complete: 86 contracts → 80 named story/request/service entries | Verified Q/M guidance, distinct material counts, optional preparation hints and receipts; deeper dialogue/lineage/branches pending | Mixed item-and-coin grove recipes remain unavailable; preparation services do not earn story/daily units | Source/native parser and encounter visibility checks; each full-world journey remains pending in the priority roadmap |
| Remaining active areas | — | Native fallback; optional sidecars integrate incrementally | Existing discovery and terminal receipts; native giver rows appear after physical encounter | Existing offering limits | Previous daily qualification; full semantic mapping unclaimed |

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

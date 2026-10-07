# Discovered-zone daily quests

A character discovers a playable area on the first genuine visit in the current
season. Discovery is a separate achievement and opens that area's private quest
journal, including areas with no tracked quests. It does not change the distinct
quest denominator or public ranking. Walking, following, login, and legitimate
travel count; temporary map inspection, staff remote commands, staff characters,
arena rooms, and ship interiors do not grant discovery.

**Active economic accounting is required** for zone journals, discovery/encounter
events, zone-story achievement surfaces, and new daily eligibility. The verified
native accounting authority supplies this gate; setting an environment flag
cannot substitute for lifecycle activation. Exact native receipt recovery and
deleted-character cleanup remain available while inactive.

Daily quests remain disabled by default. After accounting is active, set
`ZONE_STORY_DAILY_ENABLED=true` on an isolated server to enable daily eligibility
and player surfaces. Discovery and the private journal are independent of this
additional daily switch, but still require active accounting. While inactive,
ordinary quest/score omit daily output and explicit zone/daily journal commands
explain the accounting requirement. No production setting or database is changed.

## Player commands

| Command | Result |
| --- | --- |
| `quest zone <area>` | All tracked quests in a discovered area, original offering objectives, story completion, and daily status when enabled. |
| `quest daily` | Discovered-area overview, completed daily count, renown, and UTC reset countdown. |
| `quest daily <area>` | All daily candidates in that area, with Available, Locked, or Done today status. |
| `achievements zones` | Discovery count and seasonal distinct quest progress. |
| `achievements zone <area>` | Discovery achievement and existing 25/50/75/100% quest milestones. |
| `score` | Compact daily completion and renown reminder when enabled. |
| `leaderboard quests [page]` | Existing worldwide unique-quest ranking, with its 12-hour publication delay. |

Area lookup is case insensitive and accepts unambiguous partial names. Areas
with identical names show a qualifier in their journal, daily overview, and
achievements, such as `Ceothia (ceopast)`. Use that full qualified label to select
the exact area. An ambiguous command lists usable matching labels; a numeric
zone/VNUM is not required. Large
journals use the existing pager. An undiscovered area reveals no checklist or
quest progress. Commands are read-only: viewing a journal never creates an
assignment, writes a completion, rerolls a quest, or grants a reward. Ordinary
`quest` and `score` omit daily output when disabled; an explicit `quest daily`
request explains that dailies are disabled.

## Content and evidence

Builders can supply optional [per-area story mappings](../guides/ZONE_STORY_BUILDING.md).
Mapped alternatives count once in story achievements and daily checklists;
reviewed exclusions/services retain native gameplay without contributing
story/daily totals. Raw receipts are preserved. Live item/equipment steps
describe current possession, not personal acquisition history. Unsupported
native offering shapes display an explicit turn-in-unavailable message.
The [integration plan](../design/ZONE_STORY_INTEGRATION_PLAN.md) tracks later
dialogue, provenance, mixed-offering, and client presentation capabilities.

Every daily-suitable definition in a discovered area is considered each UTC day;
there is no random assignment or rotation. The original quest giver, offering,
rewards, item sources, and zone reset rules supply the gameplay. The journal is
not an additional quest engine and does not reset world content at midnight.

Catalog revision 2 retains all 2,668 stable static Q-contract identities. Of
these, 2,659 contribute to zone quest achievements and 2,133 pass the content
filter for potential daily use. Candidates still require suitable completion
evidence and the recipient's own eligibility; these counts are not a claim that
all candidates are currently available to every character.

Excluded daily content includes 9 administrative contracts, 131 contracts that
return an offered item, 70 disappearing-giver contracts in areas with no reset,
39 contracts with no repeatable item offering, and 286 contracts whose offering
shape the current accounting turn-in path cannot accept. That path supports up
to 14 exact item offerings; mixed currency/type offerings and larger contracts
remain story-only until their native offering path is supported. These remain catalogued for
ordinary quest execution and appropriate story progress. A disappearing giver
can be repeatable when the owning area normally resets. Completing a step does
not assert that an entire storyline is finished.

The default evidence policy requires at least 20 attempts from 5 distinct PIDs,
at least one success, accessible observations at the current revision, known
party context, and no carried, inaccessible, or stale-revision evidence. The
recipient must be within the observed level range, have a successful accessible
observation for their exact racewar faction, and meet any authored prerequisite
IDs. Evidence from the future cannot unlock an earlier completion. The strongest
party member must be at most 10 levels above that recipient. Definitions have an
explicit prerequisite list; existing Q contracts declare none beyond their
actual offering requirements. No prerequisite is guessed from dialogue.

```text
python3 scripts/zone_story_quest_daily_report.py \
  --observations /path/to/zone-story-observations.jsonl --format text
```

The offline report summarizes real observations and the configured policy.
Telemetry is indexed by definition at runtime so a journal does not repeatedly
scan unrelated worldwide history. See the [catalog and scripted-content audit](ZONE_STORY_QUEST_CATALOG.md)
for the exact content boundary.

## Credit and rewards

Days are `floor(unix_time / 86400)`, resetting at 00:00 UTC. Discovery, distinct
story credit, and renown are per PID and season. Each qualifying quest can mark
one daily checklist entry per character per day. Repeating it on that day adds
activity history without increasing the daily or seasonal numerator. Repeating
it tomorrow marks tomorrow's entry without increasing seasonal distinct credit.

The first qualifying completion anywhere in the world earns **one renown** for
that character and UTC day. Later quests still mark their checklist entries but
do not grant another bonus. The durable reward key remains `season:pid:period`.
Original item, coin, skill, and XP rewards retain their existing accounting and
recovery behavior; this feature adds no currency or duplicate reward path.

At offering admission, the v6 reward continuation freezes the original completion
time, season, catalog revision, policy revision, same-room credited recipients,
and subset eligible for daily credit. Every recipient is checked using their own
level, faction, discovery, prerequisite progress, and party context. Recovery
uses those frozen terms and the original offering identity, including after a
UTC boundary or policy change. A repeated receipt cannot acquire new recipients
or additional renown. Older continuations (v1-v5) remain readable but cannot
invent retroactive daily eligibility. Legacy assignment records remain readable;
new activity creates no assignments and there is no retroactive claim command.

## State compatibility and operations

SQL installations need immutable migration **0056_discovered_zone_daily_state**
after the accounting migration history. It preserves v1 data and allows v2
records in the existing table. It changes CHECK constraints without changing
columns or historical migration files. Its verifier and manifest checksums are
part of the runtime compatibility contract. The supported staging-0045 and
master-0031 histories remain separately recognized and also append migration
0054 without rewriting their sealed prefixes.

On a validated legacy load, stable quest credit is retained, discovery is
backfilled only from recorded completion rooms, and daily availability starts
at the next UTC boundary. No earlier event earns a new daily reward. A cold load
of current state reconstructs frozen daily entries and committed reward keys
without awarding anything again. Unsupported or corrupt state prevents tracking
bootstrap; a failed write rolls back in-memory progress before any discovery
announcement or completion acknowledgement.

Persistence saves changed records rather than the complete global history:

- SQL uses 255 deterministic buckets in `zone_story_quest_state`, each below the
  MEDIUMTEXT limit, updated in one guarded InnoDB transaction. Changed buckets
  use fast compressed snapshots with a bounded, validated decode; small buckets
  remain plain record documents. Dynamic queries avoid the legacy `qry`
  stack-buffer limit. Legacy aggregate row 1 is converted only after a validated
  application load. Snapshot replacement physically removes obsolete records.
- Flat-file authority retains `FLATFILE_ROOT/domains/zone-story-quests.state` and
  its private-directory/lock rules. Container version 3 is an append journal of
  bounded, checksum-protected frames covering both header and record payload.
  A torn final append is ignored and truncated before the next write; a complete
  corrupt frame fails closed. Snapshot compaction uses one atomic rename.
  Versions 1 and 2 remain readable.

Deletion removes the PID's discovery, checklist, identity, and telemetry facts
and physically compacts flat-file history. Tombstones prevent delayed receipts
from reviving the PID. Other group members retain their own credit, daily facts,
bonus keys, and first completion times, including delayed public ranking after
restart. The lifecycle inventory retains its existing protected storage routes.

The capacity regression retains 25 characters completing four quests every day
for 60 days, saving every completion and cold-loading all 6,000 events. This
exceeds the previous 16 MiB aggregate SQL ceiling. Run it on the target storage
to establish local latency; indefinite history and much larger populations still
need an operational retention budget. The feature does not silently discard
accounting or recovery history.

Measured build, gameplay, recovery, migration, and capacity results are in the
[feature qualification record](../design/DISCOVERED_ZONE_DAILY_QUESTS_QUALIFICATION.md).

For deployment, integrate accounting first, update this feature branch to that
accepted base, run focused build/gameplay/recovery validation, then apply the
new migration to the intended development installation. Production migration
and activation are separate owner-authorized operations. Once v6 receipts or
version-3 flat-file state exist, older binaries must not be used to read them.


Church ownership correction: four native candidates now belong to Church878, while Kelek879 keeps its smith candidate. Stable IDs are unchanged; prior879 receipt payloads and frozen daily recipient masks remain history. Pending committed recovery accepts only the reviewed predecessor/current revision with frozen context and a stable ID. Discovery never backfills from the owner correction. Policy remains disabled by default; enabled reviewed policy still requires accessible telemetry, observed level/faction, known party context and actual source availability. Rollover does not replenish one-cap mode-two sources.


Domain availability qualification: both native hand-ins remain potential daily candidates, with policy disabled by default. Boadwyn's required contract/band have no producer in the reviewed sources; Bal Sagoth's exact skulls are capped stock rather than ordinary CARVE results. Due mode-one resets use descriptor-based emptiness and do not guarantee source replenishment at rollover. Supplied exact items retain native acceptance without invented personal recovery. Ihsahn's separate training purchase refuses while accounting is active and is not a journal/daily objective. The [Domain dossier](../design/zone-stories/DOMAIN_OF_LOST_SOULS.md) records source restoration and combined service settlement prerequisites before broader activation.


## Drifting Realm daily qualification

The [Drifting Realm dossier](../design/zone-stories/DRIFTING_REALM.md) preserves two native candidates: four exact identical shards and one each of five different skulls. No D retirement or native prior-receipt gate applies. Mode0 supplies initial fresh-boot stock without an ordinary boot reset timer; conditional/manual renewal remains separate. Foreign skull sources have independent mode1/2/cap1 availability. Discovery or a day change does not renew local keys, proofs or giver stock. Keep policy disabled by default until READY-accounting owned source/hand-in/renewal journeys qualify actual availability, atomic outcomes, rejection and recovery. Neither optional earlier context nor ordinary reward possession adds a daily unit.


## Clavikord daily qualification

The [Clavikord dossier](../design/zone-stories/LIZARDMAN_SWAMPS_OF_CLAVIKORD.md) retains two candidates because mode1 is resettable, although each recipient departs after accepting a different head. Bemon is also the source of Vornin's proof; finishing Bemon's own return can remove that stock within the same appearance. Vornin has a25-percent declaration, and both givers wander. Qualify owned NPC/source renewal, caps/chances, active READY exact returns and retirement/recovery independently of discovery/UTC rollover. The merchant's alternate Sslith head and supplied proofs remain valid origins without personal-kill history; unavailable accounting-era trading is not a daily route. Policy stays disabled by default.


## Tower of High Sorcery daily qualification

The [Tower dossier](../design/zone-stories/TOWER_OF_HIGH_SORCERY.md) retains two mode1 candidates with independent three-material and two-part recipes. Both recipients depart; Zbarnos wanders from dispersal with a possible no-exit sink. Labyrinth room/G/P-with-equipped-sack sources and Tower E/G parts have independent caps and custody. Qualify active READY exact bundle admission, eligible party/XP terms, XP/coin/item child settlement, actual recipient retirement and owned source renewal across zone boundaries. Discovery/UTC rollover does not supply materials, remove a curse, guarantee three personal kills or return an absent giver. Potential daily classification is not guaranteed availability; policy remains disabled by default.


## Vecna's Tomb daily qualification

The [Vecna dossier](../design/zone-stories/VECNAS_TOMB.md) retains two potential candidates with mode0 sources and D recipients. Qualify active READY exact input admission, eligible party credit, individual item reward/save settlement, actual recipient retirement and owned root/giver generation renewal. Custom undead rebirth, committed epic claims, reset requests, SQL percentage/hourly retries and completed resets are different episodes. Discovery or UTC rollover supplies no material, survival history, personal defeat or returning lich. Policy remains disabled by default.


## Killing Fields daily qualification

The [dossier](../design/zone-stories/KILLING_FIELDS.md) retains one potential candidate with a container-stocked note and departing, moving giver. Qualify active READY exact input, frozen eligible party completion credit, actor currency child/save settlement, actual D retirement and owned note/container/giver renewal. Mode1 age/emptiness, reset caps and completed source construction remain separate from daily rollover. Discovery supplies no note, decoding, personal recovery, rescue or guaranteed shop. Policy remains disabled by default.


## Magma daily qualification

The [dossier](../design/zone-stories/MAGMA.md) retains one potential candidate with a rare moving giver/source and mode0 renewal. Qualify active READY exact input, frozen eligible party history, actor three-item child settlement/save, D retirement, actual admitted entry/return and owned heart/giver renewal. Neither a control-room percentage name nor UTC rollover proves fresh stock. Public route and played qualification remain pending; policy stays disabled by default.


## Mazzolin daily qualification

The [dossier](../design/zone-stories/MAZZOLIN.md) retains one potential candidate with five exact source pieces and a retiring giver. Qualify active READY joint input, frozen eligible party history, actor two-item child settlement/save, actual D retirement, protected entry/return and owned mode1 renewal for the giver and all five sources. Duplicate shards, a rune-stone claim, hidden-root discovery or UTC rollover cannot establish a fresh complete bundle. Policy stays disabled by default.


## Myconid daily qualification

The [dossier](../design/zone-stories/MYCONID.md) retains one potential candidate for an exact external spore and 50 platinum, with a moving/no-D giver. Qualify active READY acceptance, frozen eligible party history, actual actor currency settlement/save/recovery, exact external source stock/global cap 1 and completed renewal. Green-key/cache progress, mushroom operation, foreign jar return and UTC rollover do not establish a fresh spore or settle the local reward. Policy stays disabled by default.


## Vargan II daily qualification

The [dossier](../design/zone-stories/V2.md) retains one potential candidate with three hidden/NORENT exact pieces, one sword reward and a no-D giver. Qualify active READY joint acceptance, frozen eligible party history, actor reward-child settlement/save/recovery, physical stock policy and completed mode2 giver/source renewal. Key/map discovery, campaign advice, hazards and UTC rollover cannot establish a fresh bundle or settle the reward. Policy stays disabled by default.


## Phantasmagoric Caverns daily qualification

The [dossier](../design/zone-stories/VALDRAK.md) retains one potential candidate with two exact weapons and hidden heart, currency/XP terms and D retirement. Qualify active READY acceptance, frozen party history versus actor currency/actual capped XP recipients, settlement/save/recovery, giver/follower lifecycle and completed cap1 source/giver renewal. Garden, spring, shopping, ritual prose and UTC rollover cannot establish a fresh accepted bundle. Policy stays disabled by default.


## Bronze Citadel daily qualification

The [dossier](../design/zone-stories/BCTDL.md) retains one potential seal candidate; the heart is story-only. Mode0, NORENT key custody, shared cube access, active accounting reset item refusal and deferred epic/reset generation require qualification. Actual READY acceptance, reward root/recipient settlement/save/recovery, retirement and completed accountable source/giver renewal are required. Daily policy remains disabled; UTC rollover, a reset request or opening a shared route does not prove renewal.


## BrimStone Forge daily qualification

The [dossier](../design/zone-stories/BRIMEFORGE.md) retains two potential candidates. Mode2 scheduling does not prove fresh accounted lockets or rune availability. All distinct loose roots must be accepted together; actual reward item/XP entitlements, recipients/save/recovery, NORENT key lifecycle and completed accountable source/giver renewal need qualification. Daily policy remains disabled and new tracking requires active READY accounting.


## Bugger Caves daily qualification

The [dossier](../design/zone-stories/BUGGER.md) retains one meaningful potential candidate and excludes the wrong-food response. Native self-exchange is already daily-ineligible; exclusion also prevents achievement/story progress. Mode2 timing and reset caps do not prove fresh READY eggs or parent/child generations. All three exact loose roots, actual recipient armor settlement/save/recovery, NORENT egg lifecycle and completed accountable renewal require qualification. New tracking requires active READY accounting and daily policy remains disabled.


## Dirk’nspire daily qualification

The [dossier](../design/zone-stories/DIRKN.md) retains two meaningful potential candidates. Mode2/lifespan10..20 and caps do not prove fresh READY paper/document stock or giver/parent/child generations. Actual global moneybox parent selection, admitted search/lock/GET, supplied custody, native scarab/wallet recipient settlement/save/recovery and completed accountable renewal require qualification. Foreign Foe Hammer bounties keep their Alatorin identities. New tracking requires active READY accounting and daily policy remains disabled.


## Storm Port daily qualification

The [dossier](../design/zone-stories/STORMPORT.md) preserves two potential candidates. Mode2/lifespan40..50 and cap1 proofs do not establish fresh accountable stock or old-ship badge access. Foreign Tchan/giver generation, actual desk parent UID, hidden object reveal/GET, supplied custody, NORENT logout and reward recipient/save/replay renewal need qualification. Modern ferry ticket purchases and Tchan prayer refuse active accounting; neither is a hidden required step. New tracking requires active READY accounting; daily policy remains disabled.


## Arachdrathos city daily qualification

The [dossier](../design/zone-stories/ARAC-WEB.md) preserves the existing coin-only unit’s exclusion: No repeatable item offering. It contributes no achievement and no potential daily. Mode2/lifespan15..25, cap1 keeper/key stock and a retired giver do not establish fresh accountable renewal. Coin-only purchase and Body Traders services refuse active accounting; dynamic bartender quota does not turn that service into a zone daily. Require active READY accounting for new tracking; daily policy remains disabled.


## Arcium daily qualification

The [dossier](../design/zone-stories/ARCIUM.md) preserves one native achievement and one potential daily. Mode2/lifespan25..35, six cap1 heart sources and periodic timing do not qualify fresh accountable stock. Active authority guards native O/P/G/E construction. Exact six-root custody, hidden reveal/recovery, supplied transfer, Joji generation, joint reward/save/retirement/replay and completed renewal need played qualification. Keys and dream access remain separate from the offering; source-complete mapping cannot enable guarded stock or paid epic learning. New tracking requires active READY accounting; daily policy remains disabled.


## Future Ceothia daily qualification

The [dossier](../design/zone-stories/FUTURE_CEOTHIA.md) preserves one achievement and one potential daily. Empty-only mode1/lifespan40..50 and cap1 keys/devices do not prove renewed accountable sources. The requested vial has no explicit selected-source producer, and active authority guards native O/P/G/E construction. Qualify exact source/transfer/custody, giver generation, mixed key/currency/save/retirement/replay and completed renewal before activation. New tracking requires active READY accounting; frozen committed recovery is separate and daily policy remains disabled.


## Treasure Caves daily qualification

The [dossier](../design/zone-stories/TREASURE_CAVES.md) preserves one achievement and one potential daily. Mode2/lifespan10..12 and cap1 chest/key/figurine/vines do not prove accountable renewal. Active authority guards native O/P/G/E construction. Qualify source generation, root/parent/custody/transfer, persistent giver, actual coin operation/recipient/save/replay and completed replenishment before activation. New tracking requires active READY accounting; frozen committed recovery is separate and daily policy remains disabled.


## Jindon daily qualification

The [dossier](../design/zone-stories/JINDON_DEATHWOOD.md) preserves one achievement and one potential daily. Mode2/lifespan40..50/cap1 arms and persistent Sirax do not prove accountable renewal. Active authority guards item resets and legacy paid entry. Qualify actual source generation/transfer/custody, item issuance/recipient/save/replay and replenishment before activation. New tracking requires active READY accounting; committed recovery is separate and daily policy stays disabled.


## Lava Springs daily qualification

The [dossier](../design/zone-stories/LAVA_SPRINGS.md) retains one achievement and one potential daily. Mode1/lifespan20..30, rock/voucher caps and persistent giver do not prove accountable renewal. Qualify six distinct source roots or transfers, actual admitted purchase policy, batch destruction/item issuance, recipient/revisions/save/replay and replenishment. New tracking requires active READY accounting; committed recovery is separate and daily policy remains disabled.


## Lylr-Meop daily qualification

The [dossier](../design/zone-stories/LYLR_MEOP.md) retains one achievement and one potential daily. Mode2/lifespan40..50, giver cap4/scalp cap1 and D retirement do not prove renewed availability. Qualify actual access, hidden source/transfer, exact input/reward/recipient/save/replay and same-generation retirement/replenishment. New tracking requires active READY accounting; committed recovery is separate and daily policy remains disabled.


## Halfling Silver Mine daily qualification

The [dossier](../design/zone-stories/HALFLING_SILVER_MINE.md) retains one achievement and one potential daily. Mode2/lifespan20..30, priest/balor/whip cap1 and a nondeparting giver do not prove renewed source availability. Qualify actual approach, exact source or supplied material, input/reward/recipient/save/replay and accountable source/reset renewal. Grid key stock is not guaranteed access. New tracking requires active READY accounting; committed recovery is separate and daily policy remains disabled.


## Forest of Mir daily qualification

The [dossier](../design/zone-stories/FOREST_OF_MIR.md) retains one native achievement and one potential daily. Mode1/lifespan20..25, cap1 scroll/giver, a nondeparting priest, pool activations and relic calendar cooldowns do not prove usable renewed supply. Qualify actual source or exact supplied copy, experience/token/recipient/save/replay, real movement/source availability and accountable renewal. New tracking requires active READY accounting; committed recovery is separate and daily policy remains disabled.


## The Shadow Forest daily qualification

The [dossier](../design/zone-stories/SHADOW_FOREST.md) retains one native achievement/potential daily. Mode2/lifespan25..35, source/giver cap1, D departure and actual roaming do not guarantee accessible renewed stock. Qualify usable source or exact supplied head, guardian/approach admission, crown/departure/recipient/save/replay and new generations. New tracking requires active READY accounting; committed recovery is separate and daily policy remains disabled.


## Mosswood daily qualification

The [dossier](../design/zone-stories/MOSSWOOD.md) retains one native achievement/potential daily. Mode2/lifespan40..80, beet cap2,Ijale cap1 and no departure do not prove renewed available stock or lawful repeated C1000 payouts. Qualify actual source reveal/recovery or supplied input,food choice,accepted consumption,identified wallet recipient/save/replay and fresh generations. Shop production is separate from beet supply. New tracking requires active READY accounting; committed recovery is separate and daily policy remains disabled.

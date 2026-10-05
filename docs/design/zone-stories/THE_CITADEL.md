# The Citadel: apprentices, runes and guarded treasures

Priority99 of the original220-zone roadmap. This source-comprehensive mapping
covers all three contracts, dialogue, local world/reset records and relevant
shared/foreign execution. It does not claim played source, access, key destruction,
wallet or persistence qualification. New progress requires active, ready accounting.

The [schema3 journal](../../../areas/story/citadel.story.json) contains two
independent stories,20 contacts/63 verified aliases, two optional material rows
and one explicit exclusion. The [audit](../../reference/zone-story-audits/citadel.md)
preserves native identities and source evidence. No native repair ships here.

## Source closure

| Source | Full review and implication |
| --- | --- |
| [Quests](../../../areas/qst/citadel.qst) | Three Q contracts and39 M dialogue families. Exact inputs, rewards, empty placeholder and giver retirement reviewed. The twelve-item dialogue does not add goals to the one-note recipe. |
| [Rooms](../../../areas/wld/citadel.wld) | All200 records13000–13199,66 full prose/8 headers/7 metadata families,475 exact exits/98 relative patterns/106 full exit-text families. Sole boundary13000S↔12796N skulldrach. Four drawn runes, translated landing clue, underwater greenhouse, keyed vaults, secret washroom route and reciprocal returns reviewed. Registry range12973–13199. |
| [Mobiles](../../../areas/mob/citadel.mob) | All31 full prose/ID-paired numeric records13000–13030 (28 prose families). Wandering giver has21 starting resets, including repeats in the same room; actual placement can move. Prototype13025 has dialogue but no current local placement. Teachers and guardians are distinct from quest outcomes. No direct local procedure assignment or literal local numeric binding found in maintained source; six-digit Vecna IDs belong to another zone. |
| [Objects](../../../areas/obj/citadel.obj) | All63 complete records13000–13062. Every type/flag/value/effect/extra reviewed, including all book pages, drawn runes, coat/key/notes chest, twelve vault keys, four store keys, Amberyl’s lyre case and small reward key/chest. No local type25 portal or type29 switch. Imported358/67239/55187 full canonical records reviewed. No object-zero declaration in active world. |
| [Resets](../../../areas/zon/citadel.zon) | All343 commands: D84/O30/P22/M172/E30/G5;328 exact/parent-aware and106 complete expanded argument/location families, noF.341 chance100/two chance35. All caps/conditions/placements/containers and mode1 reviewed. |
| Shared execution | [Magic speech](../../../src/cmd/actcomm.c), [OPEN/UNLOCK/PICK](../../../src/cmd/actmove.c), [container flags](../../../src/core/defines.h), [quest loader/goal matching/settled continuation](../../../src/world/quest.c), [guarded runtime](../../../src/world/zone_story_quest_runtime.c), [imported bindings](../../../src/specs/specs.assign.c), [church-door effect](../../../src/specs/specs.unique.c) and [epic stone](../../../src/world/epic.c) reviewed alongside unchanged custody/reset paths from preceding audits. Magic speech is unrecorded shared access; coin rewards are recoverable quest outputs, not fees. |
| Foreign closure | Global active registry exit/object/portal/reset/recipe scan: only two usable local touching recipes, two foreign diamond stock groups and the reciprocal forest boundary. Full foreign rooms12796/78781/83537 and halfling83316 stock context reviewed.713 portal declarations scanned; none targets a local room. PlaceholderI0 is not a foreign item link. No local shop file. |

## Exact requests and progression

| Contract | Exact input → output | Projection |
| --- | --- | --- |
| Wandering apprentice13003/Q47 | I13006→C50000;D1 | Bluish key→50platinum, retire this giver; one story/potential daily. |
| Lost apprentice13030/Q260 | I13033→I13059;D0 | Notes→small key, giver remains; one story/potential daily. |
| Lost apprentice13030/Q267 | I0→empty;D0 | Unusable placeholder excluded, native identity/history retained. |

Enter through the onyx word door, learn the apprentice leads and find the arctic
coat’s key. The fixed secret-room chest holds notes and a ruby ring. Its state15
means closeable/deprecated HARDPICK/closed/locked, without PICKPROOF; a supplied,
picked or already-open path can replace personally finding a key. Giving the key
to the wandering apprentice consumes it. A completion receipt cannot restore it.

Decode the four rune drawings with Growler’s W/D/B/T pages and the riddles.
Their actual last-keyword passwords are wind/dark/black/time. Generic SAY clears
locks and matching reciprocal locks, without opening the closed door. The landing
says master but currently responds to shalafi. Neither hearing a clue nor saying
a word dispatches a new personal story event. Silence, speech ability and shared
already-open access retain native behavior.

The twelve vault contents are nightstone key13001, paper boat13030, phantom
boots13031, diamond13036, sapphire key13002, flask13028, darkstone ring13029,
silver key13003, earring13025, katana13026, mask13027 and onyx key13004. Their
guardian keys13012–13023 remain distinct. Four of those treasures key additional
store rooms; they are not goals in Q260. If a builder chooses the stated larger
haul, twelve treasures plus notes is thirteen inputs, within the current14-item
durable offering limit. Changing the accepted recipe would still change gameplay
and requires separate intent, source, consumption and news review.

The lost apprentice starts at13188 and takes only the notes. The rewarded key
fits chest13058 on the sleeping-chamber desk13189. Its state29 includes PICKPROOF;
the key’s native break setting is100percent after successful unlocking. Necklaces
13060/61 each have a35percent P-reset chance; wand13062 has100percent, all cap1.
Opening this chest, taking its contents and rescuing the apprentice are separate
from the delivery receipt. No mandatory personal earlier receipt is invented.

Amberyl13012@13134 carries key13041 (10percent break) for lyre case13040. Her
book story does not implement soul release. Stone Guardian13024@13185 carries
13044 for the study/sleeping doors. Master13017@13190 carries paradox key13042;
Portal Guardian13023@13198 carries golden key13043 for the final vault. Beholder
13027@13199 carries gear and imported epic/memory items. The church door67239
is equipment, with worn RUB glass armor/bless behavior and a15second timer;
its name does not make it a travel portal. Foreign diamond stock has no delivery
binding here. None creates an extra local quest achievement.

## Builder decisions and missing capabilities

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-CITADEL-REQUEST-INTENT | Lost apprentice asks for notes plus twelve vault items, while Q260 accepts only I13033. Decide whether to correct dialogue or deliberately create a thirteen-item recipe. Thirteen fits the current fourteen-item durable limit; name all exact prototypes and qualify custody, source availability and consumed keys before a separate fix/news commit. |
| ZSQ-CITADEL-EMPTY-CONTRACT | Q267 has I0, empty response and no reward; no object-zero declaration exists. Excluded from new completion credit without deleting native identity or historical evidence. Builder should confirm intended removal or replacement; an actual native repair needs its own commit and news wording. |
| ZSQ-CITADEL-MASTER-CLUE | Room13185 translates the magic word as master, but both door keywords end in shalafi. Decide intended clue/word, then make a narrow separate caption or keyword repair with before/after speech qualification. Journal explains current behavior. |
| ZSQ-CITADEL-SPOKEN-ACCESS | Generic check_magic_doors matches the last keyword, clears locked/secret bits and matching reciprocal state, then leaves CLOSED intact. It records no personal quest event. Add qualified actor/door generation and successful transition evidence for optional password episodes; already-open travel is not personal puzzle proof. |
| ZSQ-CITADEL-KEY-LIFECYCLE | Coat→bluish key→notes chest competes with spending that key for coins; notes→small key→pickproof desk chest is a separate access chain. Qualify current loose/held/worn/nested custody, supplied/picked/shared-open alternatives, breaks, durable destruction/recovery and cap renewal before promising live accounting journeys. |
| ZSQ-CITADEL-TREASURE-SOURCES | Twelve guardians’ keys have twelve destinations; four vault contents key further store rooms. Current item possession, player supply, container extraction and first native-source acquisition are different evidence. Register exact source/container generations only if builder chooses deeper personal episodes. |
| ZSQ-CITADEL-NARRATIVE-EPISODES | Amberyl’s sorrow, the lost apprentice’s plea and glyph research have no committed release/rescue/study outcome. Define deliberate noncombat/escort/performance endpoints and explain their distinction from delivery receipts before adding story credit. |
| ZSQ-CITADEL-SOURCE-RENEWAL | Mode1 resets, capped coat/key/chest/notes, retiring wandering givers and two chance35 chest children govern real availability. Qualify accounting-backed reset issuance and item renewal; a potential daily is not a guarantee of a fresh key, notes or loot. |
| ZSQ-CITADEL-FOREIGN-SYSTEMS | Diamond13036 has Caer Tannad floor and Alatorin chance15 stock. Beholder imports358/67239/55187; epic stone, worn church-door RUB glass spells and memory trophy have separate rules. Keep foreign discovery, ordinary gear effects and epic accounting distinct from local deliveries. |


Any actual zone/quest repair needs a separate named fix/news commit with narrow
before/after evidence. None is included in this feature mapping. Existing schema3
suffices for the two current deliveries; deeper source/puzzle/rescue episodes need
the planned qualified event adapters and deliberate builder integration.

## Validation boundary

Focused source/schema assertions and C++ journeys cover exact contracts, optional
custody, wrong/held/reward-only supplies, independent acceptance, excluded historical
compatibility, replay and cold recovery. Full production regression, all116 journal
journeys, maintained build, changed/staged format and preservation must pass before
publication. Played speech/key/source/reset/accounting/persistence remains pending.

Catalog116 journals/1567 achievements/1438 potential dailies/2191 rows. Native2668
definitions/fingerprint/content revision2/registry and prior115 mappings preserved.
Roadmap99/220 source-comprehensive,121 pending; The Elemental Groves next.

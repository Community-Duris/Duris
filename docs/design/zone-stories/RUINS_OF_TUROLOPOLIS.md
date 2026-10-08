# The Ruins of Turolopolis: comprehensive source map

Priority 73 of the original roadmap. **Source-comprehensive; played qualification
remains pending.** The revision-one [journal](../../../areas/story/willem.story.json)
maps all six native offerings as six independent stories and potential daily
candidates. Twenty-six contacts preserve nineteen addressed topic families and
39 aliases. Twelve optional checks cover eleven exact current materials and one
earlier memorial receipt. Discovery has its own achievement. Every new discovery,
encounter, journal update, achievement and daily credit requires **active, ready
accounting**; frozen obligations retain separate recovery.

**No native Turolopolis zone or quest repair ships in this checkpoint.** The
findings below are qualification work or builder decisions. Any implemented
repair must have a clearly named separate fix commit and prominent PR/news
trigger, before/after behavior, proof, limits and a news-ready sentence.

## Complete source closure

The active `areas/AREA` row is `willem *71-72`. Canonical zone71 has registry
7044–7253, reset mode one, levels30–35. Physical membership is rooms7100–7253,
rather than every number in that registry. Reviewed sources:

- [All 25 native blocks](../../../areas/qst/willem.qst): nineteen M and six Q.
  All nineteen M are addressed topic families, retaining39 aliases. Questions
  supply guidance; each keyword is not a quest achievement or learned receipt.
- [All 154 complete rooms](../../../areas/wld/willem.wld):109 full title/prose
  groups,17 numeric header families,22 extra descriptions across19 rooms and
  394 exits;386 numeric and38 text/keyword families. Fall fields2/5/10/90 are
  chances, without guaranteed movement or recovery.
- [All 42 mobiles](../../../areas/mob/willem.mob) and
  [56 objects](../../../areas/obj/willem.obj), including every description,
  flag, value, affect, book chapter, letter, badge/key, portal, container and
  equipment source. Required and reward kinds exist; no substitute is inferred
  from a similar name or ordinary carved part.
- [All 199 resets](../../../areas/zon/willem.zon):160 exact and164 parent-aware
  families; D52/O16/P7/M77/G10/E29/F8. The five centipedes, Kurtukr, Jalk and
  Amanthia retain their M leader; equipment after F belongs to the follower.
  There is no R, imported local mobile/object, shop, true ROOM_INN or computed
  ACT_TEACHER. Constants are one-based: ROOM_INN is BIT20=`1<<19`, ACT_TEACHER
  is BIT16=`1<<15`, ROOM_NO_MOB is BIT3=4; flag8 means INDOORS.
- Full assignment and exact numeric-reference scans find no active literal local
  object/mobile/room special binding. Historical comments naming7126/7127 as
  Bloodstone actors are not current assignments. This does **not** remove shared
  native Q/M handling or the interpreter's type25 teleport dispatch.

The bounded active-world closure reads the single complete foreign reset group
for Lothrell in Surface, its actual source room and all eight external boundary
room bodies. Ten cross-registry edges use eight foreign room IDs; the Surface
and zoo approaches are reciprocal. All713 active type25 portal prototypes were
scanned: six local kinds target local rooms, with no foreign-kind portal targeting
one. No dangling destination, missing required kind, imported item prototype,
foreign recipe using a local kind or relevant foreign shop was found. The
[generated audit](../../reference/zone-story-audits/willem.md) supplements full
source reading and does not prove a played journey.

Reviewed shared paths include [teleport selection and dispatch](../../../src/magic/spell_travel.c#L932),
[actual teleport movement](../../../src/magic/spell_travel.c#L714),
[interpreter teleport hook](../../../src/cmd/interp.c#L2410),
[chance-fall scheduling](../../../src/cmd/interp.c#L1898),
[falling admission](../../../src/world/falling.c#L113),
[door construction](../../../src/world/db.c#L1490),
[D reset state](../../../src/world/db.c#L4062),
[F follower generation](../../../src/world/db.c#L3952),
[NPC wandering](../../../src/mob/mobact.c#L8020),
physical encounter/discovery, exact native offering allocation, captured ownership,
indexed reward/XP/coin continuation, recipient retirement, search, unlock/open,
key destruction and accepted item custody. Source review is not operational
qualification of any of those paths.

## Six offerings and optional progression

`E` is nominal experience before actual credited-recipient/group policy; `C` is
the declared coin award. IDs here are maintainer bindings; player copy uses names,
colours, directions and actions.

| Native block / recipient | Exact requirement → declared reward; departure | Journal meaning |
| --- | --- | --- |
| [27 / Corwyck7103](../../../areas/qst/willem.qst#L27) | I7103 → I7100 + E25000; D1 | Unicorn horn → iron bunker key. Acceptance narrates cleansing, without a fountain/animal mutation |
| [117 / Lothrell7121](../../../areas/qst/willem.qst#L117) | I7124 + I7116 + I7113 + I7107 + I7110 → I7131 + E500000; D1 | One each brown/green/blue/red/black Glory Badge → lesser bloodsaber and memorial. White7130 and five copies of one colour are not substitutes |
| [149 / emissary7125](../../../areas/qst/willem.qst#L149) | I7135 → I7137 + E50000; D1 | Minotaur's time-worn letter → blue ribbon laced with silver thread. The letter is not an ooze-offering reward |
| [165 / Kurtukr7126](../../../areas/qst/willem.qst#L165) | I7131 + I7139 → I7138; D0 | Lesser bloodsaber + exact caecilia stinger → greater bloodsaber. Earlier Lothrell history is optional |
| [204 / minotaur7130](../../../areas/qst/willem.qst#L204) | I7141 → I7142 + E10000; D1 | Exact hardened blue-ooze lump → tightly bound gauze wrappings. Departure can discard the source letter |
| [273 / slave7140](../../../areas/qst/willem.qst#L273) | I7154 → C200000; D1 | Willem's loaded named skull → coins and the slave's departure. No all-prisoner liberation, escort or spell-field endpoint |

The journal gives five **separate** count-one carried-item steps for the badges;
one ANY-kind list with countfive would falsely admit five badges of one colour.
Native allocation uses all five distinct loose roots. Eleven current-material
checks are optional preparation; none records first recovery, personal defeat,
learned dialogue or offering completion. Worn and nested items are not loose
inputs. Exact supplied ingredients are legal without fabricated source history.

The main optional chain is **five badges → lesser bloodsaber + caecilia stinger
→ greater bloodsaber**. Its two accepted outcomes remain independently owned.
The earlier memorial receipt cannot restore a consumed, equipped or handed-away
lesser blade. The white badge, abandoned Eviscerator7128, two eternal flames7146,
hidden-chest scrolls and book chapters support exploration without invented new
native endpoints. No service or exclusion classification is needed for these six
pure-item requests; availability must still be qualified before live daily renewal.

## Statues, badges and exact access

The five statues form a north–south row in the **Plaza of Statues**, rather than
being tower-corner switches. CMD_RUB259 activates each type25 statue; TOUCH320
does not match. Values use fixed target, command259, charges-1 and zero; -1 is
unlimited. Each mirrored room's portal7155 uses ENTER7 and returns to plaza7145.

| Statue source / appearance | Target, loaded hero and exact badge | Mirror hatch toward7158 |
| --- | --- | --- |
| [7106 at7151](../../../areas/obj/willem.obj#L94), battle-ready woman; southern end | Gorma7105 at7140; red7107 E24/cap1 | Up, secret/pickproof; key7107 |
| [7109 at7148](../../../areas/obj/willem.obj#L140), cloaked figure; just south of centre | Okhell7106 at7159; black7110 E24/cap1 | South, secret/pickproof; key7110 |
| [7112 at7145](../../../areas/obj/willem.obj#L189), Mountain Dwarf; centre | Kalmidor7107 at7164; blue7113 E24/cap1 | North, secret/pickproof; key7113 |
| [7115 at7142](../../../areas/obj/willem.obj#L236), staff-bearing woman; just north of centre | Chlyridia7108 at7165; green7116 E24/cap1 | East, secret/pickproof; key7116 |
| [7123 at7170](../../../areas/obj/willem.obj#L367), rogue; northern end | Ballic7110 at7172; brown7124 E24/cap1 | West, secret/pickproof; key7124 |

All five badges are ITEM_KEY18 with break ratezero. Ordinary unlocking retains
them; the all-five offering consumes them. SEARCH the local secret hatch, UNLOCK
with its exact colour, OPEN and travel, or ENTER the return portal. Central7158
has locked pickproof sides with the same matching keys and an open upper exit
to7145. Keep needed badges for access before giving all five away.

Raw `.wld` door7 is masked by `setup_dir` to constructor type3; it is not directly
the runtime flag set. D6 supplies secret+locked state at the outer hatches, while
D2 supplies locked state at the central sides. Successful command handling,
selected target, new teleport arrival, first access and another player's already
opened door remain different facts. Portal targets all exist. Shared arena,
selection and movement conditions still matter; these statues do not automatically
carry followers through the special single-target follower path.

The five source actors are living trapped heroes. Lothrell's dialogue asks for
the remains of presumed-dead comrades. Treat this as a narrative/design mismatch,
not proof that native badge acceptance is broken. A builder must choose clearer
memorial wording or an explicit rescue/escape campaign before adding such credit.
Native acceptance presently permits supplied badges and lacks a personal-rescue
predicate.

## Source identity, branch choice and renewal

- [Corwyck's source](../../../areas/zon/willem.zon#L257) is the druid's home7139,
  above park7133. The unicorn7123 at stables7205 carries horn7103/cap1. It is a
  loaded wand-kind item, not an ordinary CARVE part. Iron key7100/break100 opens
  the bunker7106 north↔7120 south. The fountain7102 at7138 has static liquid28;
  the reward continuation does not cleanse it or alter mutated animal prototypes.
- [The sentinel source](../../../areas/zon/willem.zon#L328) contains two7124
  placements. Only the second carries crimson key7134/cap1/break100. It opens
  the pickproof palace7206 north↔7208 south; the iron key is not a substitute.
  Jailor7132 at7237 carries jail key7143/cap1/break100 for four pickable locked
  cells7238–7241. Prisoner7133 starts at7242 with routes to those cells; no
  accepted escort/escape endpoint follows merely opening a door.
- The secret shop7117 down↔7230 up and restaurant7163 down→7211 up routes require
  local SEARCH then OPEN. The restaurant's return is ordinary. The
  [caecilia7129 at7227](../../../areas/zon/willem.zon#L338) carries hidden
  stinger7139/cap1. Recovery and later delivery are separate accepted events.
- [Four blue oozes7115 at7155](../../../areas/zon/willem.zon#L262) share a kind,
  but only the third placement receives hardened blue lump7141/cap1. Other
  coloured oozes and black lump7126 do not substitute. Neither reset metadata
  nor an ooze encounter guarantees that an exact root is available.
- [Minotaur7130 at7231](../../../areas/zon/willem.zon#L341), a sewer dead end,
  carries letter7135/cap1, without ITEM_SECRET. Q204 retires him after continuation: shared
  [retirement](../../../src/world/quest.c#L1117) destroys all remaining gear and
  carrying before actor extraction. The letter is not issued or returned by that
  recipe. Killing him to loot the letter removes the same current ooze recipient;
  accepting ooze can discard the letter. Ordinary GET from a living carrier is
  not assumed. Qualify successful stealing/transfer, corpse recovery, supplied
  or earlier-generation letters and recipient renewal independently. This is a
  source/recipient-generation conflict needing builder intent, not a silently
  repaired dependency or a mandatory personal-kill route.
- [Kurtukr follows Sprecken](../../../areas/zon/willem.zon#L312) from7199; his
  following smith boots are real equipment. Their holding room can disperse into
  public houses7113/7119/7156, another holding room7201 and no-exit sink7200.
  The shaman starts at7199; Orvalus/wood drake at7201. Mishka7252 has bedroom
  routes7250/7251 and sink routes. These are real100-percent initial placements,
  without an invented custom handler or missing prototype. INDOORS8 is not
  ROOM_NO_MOB4, so public wandering is possible. F marks followers sentinel
  while retaining their actual leader. Current survival, path, sink residence,
  extraction and reset renewal require qualification; intentional rare dispersal
  should not be called permanent unavailability or automatically relocated.
- [Willem7138 at7253](../../../areas/zon/willem.zon#L356) carries hidden named
  skull7154/cap1, with Jalk and Amanthia following. The servant/slave7140 at7236
  accepts that exact kind. Ordinary carved skull7 is different; supplied proof
  does not require recorded personal defeat or freeing all prisoners.
- The adventurer7120 at7100 carries book7129, whose three complete chapters
  describe history. LOOK, questions, the earlier cloud and threatened attack on
  Winterhaven are lore without accepted chapter-learning, time travel, siege,
  orchestra rescue or field-collapse outcomes.

Key break100 is a native rate on successful unlock, with separately admitted
destruction. A reset lock may need another physical copy; an earlier key receipt
does not replenish it. Falls at7139/7162/7219/7246 have chances2/5/10/90 and valid
down destinations7133/7225/7218/7233. Flying/levitating, riding, climbing, alive
actor and movement/scheduling conditions still apply. Failure, forced arrival or
damage does not imply accepted recovery or voluntary exploration.

Five recipients depart; Kurtukr remains. Five cap-one badges, letter,
stinger, ooze lump and skull, plus keys and actual recipient generations, constrain
availability. Six daily candidates are potential units, rather than a promise of
six renewable quests every day. Nominal XP25000/500000/50000/10000 is subject to
the shared frozen per-recipient policy: offerer cap at one tenth of next-level
table value, other admitted same-room group members at the full table value.
Coins and item outputs likewise need indexed settlement/recovery. Do not label
this existing shared policy as a Turolopolis balance defect without a design
decision and executable before/after proof.

## Foreign giver and local story ownership

The sole [active Lothrell reset](../../../areas/zon/surface.zon#L122) is M7121,
cap1 at Surface room515264, **Rolling Fertile Foothills Covered With Grass**.
His local prototype and complete source-room body exist. The old Lothrell
start7198 is empty, with no selected incoming active exit or portal and six
one-way Surface dispersal edges. It does not place the current recipient there.
His narrated century in Verspin is backstory, rather than today's source.

An encounter requires discovery of the **physical** Surface room's zone5000;
meeting him does not discover zone71. An accepted memorial belongs canonically
to71 even when its recorded room is515264. A receipt cannot fabricate Turolopolis
travel. The journal remains gated on actual local discovery, then can reveal the
already encountered foreign giver. Wrong-owner5000 completion is rejected.
Current encounter hints use the physical zone and can point at Surface instead
of the native home journal; plan a truthful cross-zone navigation link after
physical/home visibility checks, rather than rewriting native ownership.

All nine relevant foreign full room bodies are closed:515264,53000 and the seven
Surface boundary destinations548452/578047/532894/551641/550095/514897/524045.
The public Surface548452↔7102 and zoo53000↔7123 approaches are reciprocal;
the remaining edges are legacy one-way dispersal. No foreign consumer or producer
uses a local quest kind in the active Q/QA scan.

## Balanced proposals and larger capability plan

| Finding | Existing behavior and uncertainty | Builder/implementation plan |
| --- | --- | --- |
| Presumed-dead comrades versus living trapped heroes | Exact badge offering works; no personal rescue predicate | Choose memorial wording or add explicit saved living-hero escape endpoints with supplied-proof alternatives. Keep five exact ALL inputs and individual badge-as-key custody |
| Fountain cleansing and palace liberation | Accepted horn/skull exchanges narrate success but mutate no fountain, ecology, field or all-prisoner state | Decide whether dialogue is sufficient or add native accepted effects/campaign terminals. Do not announce a world-restoration repair or infer it from D1 |
| Minotaur letter and retiring recipient | Letter is source stock; both death and ooze departure can remove the current recipient/material | Qualify generation-specific source transfer, destruction, recipient survival/retirement and renewed supply. Builder chooses clearer branch guidance, explicit pre-departure handoff or another native route; no reward/drop change ships |
| Rare-load dispersal and sink | Initial sources and public routes exist; sink7200 has no exits | Qualify actual master/follower wandering, alive/source stock, sink stay and reset extraction. Preserve intentional rarity until builder review chooses reset/source repair |
| Cross-zone giver guidance | Physical Surface encounter and canonical local completion are supported; physical-zone hint may miss the home journal | Add visible home-zone links while keeping discovery independent, exact room/source ownership and cold recovery |
| Stateless access/readiness versus history | RUB/ENTER, local SEARCH, exact doors, fall chance and current key/root possession do not prove past action | Accepted UID/source/custody, local reveal, before/after door state, successful selected teleport arrival and fall episodes. No per-keyword, attempted-command or damage credit |
| Daily renewal and multi-output settlement | Six pure-item units, five D1 recipients and cap-one roots; XP/coins/items have separate indexed obligations | Qualify busy/rejected/partial/replay/cold publication, per-recipient awards, actor retirement and admitted reset generations before enabling live renewal |

No universal inference can derive rescue or fountain-restoration intent from
these scripts alone. The per-zone story sidecar supplies semantic grouping and
guidance today; builder-authored ALL/ANY branches require concrete accepted
native events before richer progress cards can display them as completed steps.
Use named entry cards for the six independent outcomes, five colour checks for
the memorial, an optional producer link for the upgrade and separate access/
availability guidance. Keep current Ready/Missing, accepted history, first-source
recovery and supplied handoffs distinct across ANSI and future GMCP presentation.

## Verification and limits

Focused source fixtures lock all six recipes/owners/classifications, complete
prototype membership, topic aliases, five exact badge checks, type25 commands,
door/reset/source roles, hidden exact materials, cap-one stock and foreign giver.
Python/C++ projection journeys test physical discovery, foreign encountered giver,
read-only exact/wrong/worn supplies, one missing colour despite multiple copies,
supplied upgrade without memorial history, spent optional history, wrong owner,
six independent outcomes, replay and cold recovery. Catalog/production regression,
maintained build, changed/staged formatting, links and preservation checks are
required before publication.

Synthetic completion events do not qualify played RUB/ENTER/SEARCH/GET/steal/key/
fall/combat/offering/XP/coin/reward/retirement/reset/persistence/daily renewal. The
checkpoint changes journal/docs/tests, with no native zone/quest/runtime repair.
Original queue: **73/220 source-comprehensive,147 pending; Ixarkon next**.
The full goal remains active. No accounting activation, DB/server operation,
migration, deployment or merge occurred.

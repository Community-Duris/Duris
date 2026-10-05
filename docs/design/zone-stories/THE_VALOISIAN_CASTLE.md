# The Valoisian Castle: comprehensive source map

Priority85 of the original roadmap. **Source-comprehensive; played qualification
remains pending.** The [journal](../../../areas/story/val.story.json) uses
schema3/revision1: eight cards bind all eight native recipes. Eighteen contacts
retain all twenty-six dialogue aliases. Fifteen optional checks distinguish
fourteen current materials from an earlier cook receipt. Discovery has its
own achievement. All new tracking requires **active, ready accounting**;
frozen reward recovery is separate.

**No native zone or quest repair ships.** Preserve gates, keys, stock, reset
mode, exact recipes and mobility. The
[Fields legacy hotfix follow-up](FIELDS_BETWEEN.md#builder-required-follow-up-replace-the-legacy-rift-hotfix-safely)
remains required builder work: the owner-confirmed intentional escape hotfix
is not a pickup defect to remove. Actual repairs need separate named fix
commits, before/after evidence and prominent PR/news.

## Complete local and bounded foreign closure

Active `val`, zone384, registry38384–38586; physical rooms38400–38586 with
thirteen numbering gaps. Native header `38586 1 0 70 80 1` uses reset mode1.
Header levels are metadata, not a promise of safe access or combat.

- [All13 native blocks](../../../areas/qst/val.qst): M5/Q8, with complete
  responses, exact inputs/rewards, departure and26 aliases. No MA/QA, coin,
  type input or unsupported large bundle. The two Madam Oakencrest Q responses
  are empty; item consumption/reward identities still define distinct outcomes.
- [All174 rooms](../../../areas/wld/val.wld):120 complete title/prose families,
  17 headers,374 exact exits,145 relative exit patterns,11 full exit-text
  families and two complete non-exit metadata families. All memberships,
  directions, keys, flags and terrain reviewed. Public/private gardens,
  royal floors, shops, three inhabited estates, Veralis, taverns and roads
  have different native access and narrative contexts.
- [All93 mobiles](../../../areas/mob/val.mob):93 full prose families and every
  numeric body. No local ACT_TEACHER flag or epic-teacher table binding was
  identified. Hired hand38478 and Maxwell38491 have no local M/F placement;
  their prose does not authorize spawning or new quest endpoints.
- [All57 objects](../../../areas/obj/val.obj):38400–38456, including all types,
  flags, values, descriptions and affects. No local teleport or switch.
  Wine38424 is treasure8, plate38425/note38438/caravan38443 are trash13;
  the caravan is fixed scenery, not a vehicle controller. White rose38451
  and book38456 are worn11; the book is not ITEM_BOOK23. Their names do not
  establish drinking, eating, carriage travel or delivered learning.
- [All320 resets](../../../areas/zon/val.zon):106D/5O/142M/47E/18G/2F;
  270 exact families,280 M-parent-aware families,166 location-expanded groups.
  No imported prototype occurs in local reset arguments. All caps/chances/
  locations reviewed. [F execution](../../../src/world/db.c#L3952) replaces
  the current mob: the chamberlain's F-loaded guards wield battle maces,
  while the chamberlain wears his own silver ring.
- No literal local mob/object/room assignment or `val.shp` was identified.
  That does not prove absence of shared dispatch: room38493 carries ROOM_INN
  and [world loading](../../../src/world/db.c#L1368) registers `inn`.
  The shared rent service is separate from the assassin's quest.
  Swordsman38475's `_spec1_` selects
  [class specialization](../../../src/mob/mobconv.c#L112), not a movement
  gate procedure. Global shop data contains no matching local keeper VNUM.
  A smith's room name or carried inventory alone does not prove BUY admission.
- All active native contracts and foreign resets referencing local objects/
  mobiles were scanned. Eight touching local-item recipes are all local.
  One foreign group in [Verspin](../../../areas/zon/verspin.zon#L370) loads
  leader28113, forearm guard28140, local ebony dagger38421 and four F28114
  smugglers. The dagger is attached before F and belongs to the leader; it
  is not an input to any of the eight local recipes. Full foreign mob/object/
  room bodies were reviewed as bounded context, not a new journal endpoint.
- All713 active type25 objects were scanned; no destination into these local
  rooms identified. The sole ordinary boundary is38575 SOUTH→521073 with
  [Surface reciprocal](../../../areas/wld/surface.wld) NORTH→38575. The full
  boundary room was reviewed. These bounds do not exclude spell/dynamic routes.
- The local undirected topology has components157/2/15. Banquet rooms38440–41
  connect only to one another. Fifteen Veralis rooms38508–17/38519–23 contain
  explicit unfinished descriptions and connect only within their component.
  They have no local reset population or external ordinary entrance identified.
  This does not prove that every numbering gap is missing content or every
  isolated component is intended to be opened.

The [generated audit](../../reference/zone-story-audits/val.md) retains exact
native contracts, source/reset evidence and all addressed topics. It supplements
this full source review; neither it nor synthetic projection proves gameplay.

## Progression stories and exact sources

| Card / native recipe | Progression and acceptance limit |
| --- | --- |
| King Ulgris: [Q18](../../../areas/qst/val.qst#L18),38415/38416/38417→38418,D0 | Lords Oakencrest38419 at38545, Du’kel38420 at38496 and Berani38421 at38525 carry their respective seals. Three distinct seals together reward winged boots. Personal kills or all political rivals defeated are not predicates. |
| Hearty dwarf: [Q38](../../../areas/qst/val.qst#L38),38410/38409/38413/38414→38420,D1 | Dwarf38426 at38405 requests four models. One leggings reward, followed by departure; broad crafting dialogue does not promise four crafted outputs. |
| Stealthy figure: [Q67](../../../areas/qst/val.qst#L67),38430/38431→38432,D0 | Assassin38427 at38493 takes Karyn's and Dyneen's seals from prince38428/room38472 and princess38429/room38473. Reward dagger is a separate outcome from the king's seal bundle. No overthrow or personal royal kill proof. |
| Bakery cook: [Q86](../../../areas/qst/val.qst#L86),38424→38425,D0 | Cook38434 at38551 takes hidden cellar wine and gives the dinner plate with delivery guidance. Other cooks do not own this recipe. |
| Queen Napolia: [Q96](../../../areas/qst/val.qst#L96),38425→38428,D0 | Queen38438 at38475 accepts dinner for a royal token. Prior cook receipt is optional; a supplied plate is valid, and a prior receipt cannot replace a consumed plate. |
| Madam's beautiful roses: [Q104](../../../areas/qst/val.qst#L104),38442→38456,D0 | Floor roses in Du’kel courtyard38495 go to Madam38449 on Oakencrest balcony38546 for the book. Empty response does not imply a romantic/reading scene or learned achievement. |
| Madam's white rose: [Q108](../../../areas/qst/val.qst#L108),38451→38455,D0 | Former soldier38483 wears the white rose at38534; recover exact loose proof for the silk garment. Other roses cannot substitute. Separate accepted gift with empty response. |
| Elven ambassador: [Q124](../../../areas/qst/val.qst#L124),38438→38439,D0 | Red porter38459 at38454 carries the note; ambassador38463 waits at38451 and rewards Opticlude. Other porters/papers differ. No personal reading, translation or negotiated treaty predicate. |

These are eight story exchanges rather than extra equipment-service or dialogue
milestones. The rose recipes accept quest-marked proofs and distinct rewards;
their sparse prose is preserved as a limitation. All five response families
retain26 aliases; saying each alias does not earn26 achievements. The journal
uses fourteen exact loose-material checks and one optional earlier cook receipt.

| Exact source / access | Native evidence and limits |
| --- | --- |
| Battle mace38410 | Equipped by F38445 royal guards following M38444 chamberlain at38439; also G on royal weaponsmith38416 at38487. Same prototype is a source alternative, not two crafting achievements. |
| Polearm38409 | Equipped by pikemen38461 at38445/38449 and38436 at38464; also G on royal weaponsmith38416 at38487. No shop transaction inferred solely from G. |
| Sleeves38413/leggings38414 | G on royal armorsmith38418 at38490, cap1 each. Stock held elsewhere and actual renewal affect availability. |
| Hidden wine38424 | O cap1 floor38526; SECRET flag. Vineyard keeper38439 carries key38429 at38525; pickproof DOWN cellar gate38525↔38526 reset locked. Key breaks on successful use. SEARCH/GET, unlock and later hand-in are distinct actions. |
| Beautiful roses38442 | O cap1 floor38495; exterior pickproof NORTH gate38484↔38495 reset locked, crimson key38454 carried by swordsman38475 outside. No magical belt is implemented or inferred. |
| White rose38451 | E on former soldier38483 at38534, guild-insignia slot24; worn proof must become loose before offer. Native name alone does not prove carving a rose or killing its wearer. |
| Other keyed gates | Steel38406 from soldiers38415/38429 and guards38413/38552 opens main gate38429↔38552; ancient38407 on griffon38414/38518 opens38518↔38524. Both are pickproof/reset locked, both keys break on successful use. Preserve original design. |
| Secret garden ladders | Public garden38411 UP↔38462 DOWN and private garden38418 UP↔38478 DOWN use raw door kind5 and D reset state5: secret, closed and unlocked. Ordinary garden access38405↔38416 and38408↔38409 is closed/unlocked. Observation is not personal opening credit. |

## Required builder work and universal capability additions

| Follow-up | Evidence | Plan and acceptance before implementation |
| --- | --- | --- |
| **ZSQ-VAL-SOURCE-RENEWAL** | F-held mace; several model sources; cap1 proofs/rewards; departing dwarf; mode1 | Admit exact prototype/UID/root/custody, leader/follower and reset episode; distinguish original recovery from supplied proof. Qualify worn/nested/missing items, concurrent offers, retry/replay, reward recovery and actual source/recipient renewal. Eight static candidates do not promise eight repeatable daily runs. |
| **ZSQ-VAL-ACCESS-LEARNING** | Hidden wine, breaking keys, pickproof gates, secret closed ladders and logistics note | Record selected reader/target/passage/revision and actual delivered text separately from successful SEARCH/GET, door transition and committed arrival. Test shared unlock, key destruction, failure/no-op, gifts, already-open ladders and cold recovery. Builder decides whether any personal prerequisite is wanted. Preserve existing access and PvP policy. |
| **ZSQ-VAL-POLITICAL-ENDPOINTS** | King requests lords' demise; assassin mentions downfall; all contracts accept items only | Decide optional investigation, personal combat versus supplied seals, allegiance/branch exclusivity and meaningful final political result. Admit authoritative kill/beneficiary/group/branch/result events before credit. Do not retroactively require kills or invent king death, saved heirs or concluded treaty. |
| **ZSQ-VAL-FLOWER-EXPLANATION** | Two distinct gift inputs/rewards, both response bodies empty | Builder confirms intended scope and adds minimal truthful native response if desired. Clarify exact source and reward; do not invent relationship/sexual/reading events or add new achievement count from narration. Actual native text repair gets separate fix/news. |
| **ZSQ-VAL-UNFINISHED-COMPONENTS** |15 explicit unfinished Veralis rooms and separate two-room banquet component; unplaced hired hand/Maxwell | Establish intentional reserve/retirement versus completion. Review authored identity (Veralis versus three requested families), population and intended topology. Prefer accurate descriptions first; new connections/spawns require builder access/balance design and reciprocal/terrain tests. No automatic activation or gap-filling. |
| **ZSQ-VAL-CLUE-CONSISTENCY** | Smith/shop claims without identified keeper; broad dwarf crafting; book type11, wine8/plate13; copy/keyword typos | Confirm deliberate legacy mechanics versus mistaken text. Prefer minimal caption/alias repair to fit native acceptance; type changes can affect equipment/eating/learning and need explicit design. No implicit BUY/shop registration or new reward. Isolate any actual repair in a named fix commit/news. |

Royal retainers, diplomacy, the monk/library, champion's past dragon, nomads,
drawbridge construction and servants' debts provide supporting stories. No local
native terminal proves freeing servants, defeating Suthxx, building ships,
opening the drawbridge, discovering a political conspiracy or all treaties.
Dynamic visual progress can show current readiness, optional earlier receipts
and accepted outcomes now. Richer source/reading/access/branch facts need durable
authoritative events before richer ANSI/GMCP milestones.

## Verification and remaining qualification

Existing source/schema and Python/C++ loader/projection tests cover all eight
bindings, exact seals, distinct roses, worn proof, supplied plate without prior
cook history, spent materials versus earlier receipts, canonical outcome cards,
replay and cold recovery without fabricated foreign discovery. Full production
regression, maintained build, changed/staged format, whitespace/links and
preservation checks are required before publication. All102 earlier journals,
2668 native definitions/fingerprint/revision2/registry and original220 queue
remain intact. Eight authored cards replace eight fallback units:103 journals,
1585 achievements,1441 potential dailies and2195 rows. Roadmap85/220 complete
at source level,135 pending; Harrow -The Gnome Village (`harrow`) next. Full goal active.

Synthetic receipts do not qualify played source/gifts, F spawning, native offer
dispatch/consumption/reward settlement, SEARCH/GET/READ/key destruction,
access/combat/politics, stock/recipient renewal or database persistence. Active,
ready accounting remains mandatory and frozen recovery separate. No accounting
activation, database/server operation, migration, deployment or merge occurs.

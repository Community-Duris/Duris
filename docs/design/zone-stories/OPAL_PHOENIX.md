# Enclave of the Opal Phoenix: comprehensive source map

Reviewed October 4, 2026. Zone708, `opalphoenix`; original roadmap priority68.
**Source coverage is comprehensive; actual gameplay qualification remains pending.**
Active, ready accounting is required for discovery, visible encounters, journals
and new achievement/daily credit. Frozen recovery obligations remain separate.

The [schema-three journal](../../../areas/story/opalphoenix.story.json) covers
all three native outcomes, nine contacts and all eight addressed aliases.
Four optional checks comprise three current materials and one earlier student
receipt. The ordinary route is quill → student’s sand → Alazia’s tome, quill and
declared experience. Bear remains → grass cloak is independent. Supplied exact
items skip personal production, kills and collection history. Discovering the
zone is separate; three potential daily candidates remain without certifying
actual reward settlement, recipient reappearance or renewal.

## Reviewed evidence and scope

- All six [native blocks](../../../areas/qst/opalphoenix.qst): three M/two Q/one QA,
  including QA62, all three addressed response families and eight aliases.
  Alazia preserves `hi/hello/sand`; the student `hi/hello/lost/something`; the
  old woman `elves`. Keywords explain requests without eight achievements.
- All76 [rooms](../../../areas/wld/opalphoenix.wld),70801–70876:45 complete
  prose groups, six headers/six non-exit metadata families,101 numeric exit
  families and four complete exit-description/keyword pairs. All properties,
  descriptions and memberships were read. There are no local F/C declarations.
  The [header](../../../areas/zon/opalphoenix.zon#L3) stays
  `70876 2 0 10 11 1`, reset mode two. Registry70353–70876 is broader than the
  physical local rooms and prototypes; registry membership is not physical
  story ownership.
- All18 [mobiles](../../../areas/mob/opalphoenix.mob),70801–70818, and24
  [objects](../../../areas/obj/opalphoenix.obj),70801–70824: complete bodies,
  flags, types, values, extras and effects. Same-named students and bears have
  different actual kinds and source behavior. A container name does not
  guarantee ITEM_CONTAINER: sand70823 is ITEM_OTHER.
- All84 [reset rows](../../../areas/zon/opalphoenix.zon):M50/G13/D12/E4/O3/P2,
  all76 exact and76 parent-aware families with every member, cap, parent and
  destination. References resolve. G/E use the actual preceding live mobile;
  P resolves a matching container by kind via
  [get_obj_num](../../../src/world/handler.c#L2370), without certifying the
  preceding physical UID. Local sources have cap1; bear70816 has cap2. Stock
  G rows have cap999. Caps are declarations, without availability proof.
- Entire [shop70801](../../../areas/shp/opalphoenix.shp):Alazia70801 at70876,
  eleven kinds70801–09/70811/70817, local stock resets and singleton binding.
  Full shared shop registration/keeper/list/admission and active accounting
  trade guard were traced. Native ASK/TELL/GIVE quest dispatch is independent.
- Bounded global native consumers/producers for all24 local kinds and actual
  physical givers, reset/shop/custom literal references, assignments and all713
  type-25 teleport prototypes were checked. No foreign native continuation,
  imported reset, local literal special or fixed teleport into these rooms
  appeared. The generated [source index](../../reference/zone-story-audits/opalphoenix.md)
  includes23 pirate705xx/70501 assignments under its broad registry range.
  Those are foreign physical prototypes/rooms, rather than local Opal specials;
  the actual708xx filter has none. Computed epic-teacher and inn bindings do
  exist and were reviewed independently of the literal scan.
- The complete ordinary foreign boundary room was reviewed:
  Surface528831 west ↔70875 east. Its other cardinal exits and coastal sector
  were read; this is a bounded boundary review, without a whole Surface audit.
- Relevant actual offering, frozen item/XP continuation, reward recovery,
  recipient extraction, item creation/publication, normal visibility and named
  selection, SEARCH/GET/UNLOCK/OPEN/key destruction, reset/roaming, shop,
  computed teacher binding, room parser and inn terminal-save execution were
  traced. Source trace and projection tests do not establish played admission,
  failure recovery, output custody or daily renewal.

## All native exchanges

| Giver / Q line | Exact input → output | Actual story and source limit |
| --- | --- | --- |
| Student70802 / [Q34](../../../areas/qst/opalphoenix.qst#L34) | I70812 quill → E20000 + I70823 sand; D1 | Story. Quill P58 is in open desk70810 at70845; Alazia’s reward is a second declared source. Three students share names and room, but only70802 owns the contract. Supplied quill is valid; no exam-success requirement. Recipient disappears |
| Alazia70801 / [Q14](../../../areas/qst/opalphoenix.qst#L14) | I70823 sand → E40000 + I70811 tome + I70812 quill; D1 | Story. Student receipt explains the ordinary route but is optional. Coastal sand lore does not require a beach-source journey. One carried sand root satisfies the recipe; its container name adds no opening step. Recipient and remaining stock are extracted |
| Old woman70815 / [QA62](../../../areas/qst/opalphoenix.qst#L62) | I70819 intestines → I70820 grass cloak; D0, echo-all | Independent story. Bear70816 M111/G112 is the declared source; other same-named bears do not all have this item. Supplied exact remains skip personal kill/source history. Spell, elves and ogres explain motivation without a declared spell or victory effect |

All three recipes are item-only and use the existing owned offering/reward
path. Native nominal experience is20,000/40,000; actual frozen actor awards
are capped at the admission level using the next-level table divided by ten,
while other frozen credited party members use the full next-level cap.
These numbers do not promise60,000 to every participant. Canonical recipe
ordering differs from runtime linked-list reward order; indexed obligations
must retain the captured runtime continuation.

The quill/sand/quill cycle is real declared production, without personal-history
constraints. A prior receipt cannot restore a spent, worn, nested or transferred
input. No parent-campaign finale is invented, and two items plus experience
remain one Alazia outcome rather than three achievements.

## Sources, hidden selection and optional mask access

Quill70812 P58cap1 is inside desk70810 O57cap1 at classroom70845. The desk’s
values80/1/0/80 mean closeable and initially open, without a lock. The quill
is type21 PEN and may be held or belt-attached; an equipped quill is not a loose
offering. Student70802 M86 is a sentinel, as are the two other same-named
students70803/70804, whose presence does not supply another quest contract.

Sand70823 is type12 OTHER, with TAKE/HOLD, raw weight78 and retained NORESET.
Its original flags20480 included SECRET4096. Actual
[mortal visibility](../../../src/core/utility.c#L2830) rejects SECRET before
the own-inventory shortcut; actual
[named list lookup](../../../src/world/handler.c#L5980) offers a NOSHOW
exception, without a corresponding SECRET exception. Prototype instantiation
copies extra_flags; conversion and creation-grant publication do not clear
SECRET. SEARCH scans a room or selected visible container; it does not search
a secret carried OTHER root. Thus the generated sand lacks the ordinary
search route available to the other hidden items.

**Actual repair, separate commit [ac8e2de48](https://github.com/Community-Duris/Duris/commit/ac8e2de48f3aba3915d95abb7ee1d0fefd580f14):** sand70823 clears only SECRET,
changing20480→16384. All other prototype bytes, quest terms, rewards, source
caps and definitions stay unchanged. The focused test compiles actual visibility,
alias matching and list selection under ASan/UBSan, with an awake mortal owner.
The original prototype fails; corrected sand is visible and selects by
`large/container/fine/sand/1.sand` in both lookup modes. Wrong name, duplicate
ordinal, blind owner and a reintroduced SECRET flag are rejected. This proves
the corrected selection boundary, without a played full offering/reward cycle.
Existing saved item instances are not rewritten; operational remediation of
already-issued hidden sand needs a separate owner-approved, identity-aware
plan. The prototype repair applies to subsequently instantiated rewards.

**News-ready:** **Opal Phoenix’s student sand reward is now visible and can be
selected for Alazia’s delivery quest.** Keep this actual repair prominent in
PR/news, separate from the journal and pending capability work.

Intestines70819 retain SECRET. Bear70816 starts at70865 with G112cap1; the
second70816 starts at70874 without another G row, and distinct same-named
bear70818 at70868 has no reagent reset. Bears/doe are not sentinels and lack
stay-zone, so starting rooms are leads rather than guaranteed current positions.
SEARCH of an ordinary source corpse can reveal its hidden contents before GET;
revealing an item is not acquisition, a personal kill or accepted offering.
The old woman is a sentinel at hut70870 with a real healing-room flag.

The optional mask70822 P60cap1 is in chest70816 O59cap1 at70851. The chest
retains SECRET; its values30/13/70821/40 declare closeable, closed and locked,
with exact key70821. SEARCH the room reveals the chest. Azalea70806 at70852
has hidden key70821 G97cap1 and held staff70818 E96. SEARCH the source corpse
can reveal the key before ordinary retrieval. `has_key` recognizes the exact
loose carried or HOLD key, without scanning nested containers.

Key70821 is type18 KEY with NORENT/NORESET/SECRET and value[1]=100, a deliberate
100-percent ordinary break roll on successful unlock. UNLOCK clears the
chest’s locked bit before attempting committed key destruction. A busy/rejected
break can leave the key intact with the chest already unlocked; key cost and
world access state require separate evidence. Normal OPEN then admits access
to contents; hidden source reveal, exact key custody, unlocked/open bits and
mask GET are different facts. No native mask delivery, talisman reward or mask
achievement exists. The chest/key/intestine flags and consumable key remain
unchanged; their legitimate search/selection paths differ from the sand defect.

## Actual shared execution and lifecycle

Quest assignment sets `qst_func`, independent of a shop’s `func.mob`.
[Interpreter dispatch](../../../src/cmd/interp.c#L2818) runs mobile services and
then native quest dispatch; shop BUY/SELL/PERUSE/REPAIR/FORGE interception does
not consume ordinary ASK/TELL/GIVE. [Quester](../../../src/world/quest.c#L1752)
checks mutual visibility, awake/non-combat state and supported actor/giver,
then resolves selected loose carried input using the actual list lookup.
This explains why the sand flag was a concrete hand-in blocker.

[Item-only offering](../../../src/world/quest.c#L1517) allocates exact distinct
owned roots, captures actor/credited party and accepts a destruction batch with
frozen continuation. The [capture](../../../src/world/quest.c#L689) retains
zone/catalog owner, version, seasons/daily identities and indexed frozen XP.
[Publication](../../../src/world/quest.c#L947) verifies committed ownership and
live topology before removing inputs. [Completion](../../../src/world/quest.c#L989)
dispatches recovery of each declared item/XP obligation and other present
credited recipients; student and Alazia D1 then extract remaining possessions
and the receiver. Alazia’s eleven stock roots are not additional player rewards.

[Recovery](../../../src/world/quest.c#L1144) retains verified economic history,
frozen owner/recipient/indexed reward definitions, applied masks and trophy/save
receipts. It supports replay/cold recovery without inferring a fresh giver or
rewriting declared amounts from current level. An accepted quest receipt is
separate from each item/XP settlement, current custody, actual NPC removal,
reset reappearance and a new day’s material supply.

[Creation publication](../../../src/item/item_movement_transaction.c#L1104)
places the frozen reward in the actor’s inventory without normalizing SECRET.
[GET admission](../../../src/cmd/actobj.c#L892) validates source ownership and
submits the movement; [GET callback](../../../src/cmd/actobj.c#L519) checks
committed source topology before publishing placement. SEARCH itself does not
move custody. Original world/reset/corpse acquisition, player handoff and
subsequent transfers need identity/episode/reason evidence beyond a VNUM and
present material check. Those richer journal semantics remain a plan.

## Real services, room properties and scenery

All twelve D resets set six reciprocal exits CLOSED1, not locked:70846W↔70847E
archmage door;70849W↔70852E druid vines;70854S↔70857N monk door;
70855S↔70856N meditation door/beads;70860N↔70861S ranger door;
70861E↔70876W shop door. World flags1/key0 match ordinary OPEN behavior,
including reciprocal closed/secret clearing. There is no type29 local switch,
reset-blocked route or positive exit-key requirement. The mask’s object lock
is a separate prerequisite. Some hallways are asymmetric; nonreciprocity alone
does not establish a broken exit or justify changing topology.

The rope bridges have scenic fragility/sturdiness, without declared local fall,
current or custom failure execution. Sign70824 is type13 TRASH; evil-exclusion
and Melkivar/Opal Phoenix loyalty prose do not establish an accepted faction
gate. Exam success, monk/ranger instruction, acolyte prayer and the old woman’s
planned spell/ogre defeat have no additional native local accepted endpoint.
Do not infer that every narrative aspiration is an unfinished quest.

All18 raw mobiles lack ACT_TEACHER. Azalea still has actual computed binding:
the [epic table](../../../src/classes/epic_skills.c#L206) names70806 and
Nature’s Sanctity, max100, without a prerequisite/deny skill. Full boot
`epic_initialization` binds the table; optional/minimal boot behavior differs.
The actual [epic teacher](../../../src/classes/epic_skills.c#L453) checks typed
skill, class/level, progress, points/copper and rejects while accounting is
active before submitting the transaction. Base reward values75points/750000coins
and current multipliers/progress/class factors are not an enabled quote or
another supported native quest. Keep this limitation visible; do not activate
legacy payment paths for a journal.

Alazia’s real shop uses configured singleton identity, producing list and
eleven stock resets. The actual accounting guard refuses BUY/SELL/PERUSE/REPAIR/
FORGE; LIST/VALUE views remain subject to normal visibility/open-hour conditions.
Listing stock does not prove commerce, settlement or new quest credit.
The native sand exchange remains independently dispatched.

Room parser adds INDOORS/NO_PRECIP to sector0, and dynamically binds
[innproc](../../../src/specs/specs.room.c#L243) wherever ROOM_INN is set.
Rooms70851/70852 have flags524288;70869 flags134774784 includes the same inn
property. Hut70870 has healing131072. Actual rent checks actor state, welcome,
combat and other admission conditions, then terminal-save success before
dismissal/extraction; failure restores temporary home/trance and permits retry.
It is not universally disabled by the accounting service guard. `RENT_INN`
requires DB acknowledgement with journal handoff disabled. These unusual but
real room properties are not declared broken merely because no named innkeeper
or inn-like prose appears. Played admission/save/retry remains unqualified.

## Capability additions and balanced repair plans

| Need found | Existing safe journal behavior | Implementation / qualification plan |
| --- | --- | --- |
| Name/type, SECRET/NOSHOW and source reveal differ | Exact current material checks; search guidance; one proven sand fix | Qualify actual mortal display, indexed named selection, source visibility, successful SEARCH and current owned UID. Preserve hidden-source intent; never globally remove hidden flags from quest items |
| Same-named actors and moving source variants | Contacts identify actual giver/source kinds and starting leads | Track actual encountered NPC runtime identity and source-reset episode. A visible name or room lead does not prove the correct recipe/source or personal first acquisition |
| Cyclic item production and supplied shortcuts | Three independent outcomes; one optional student history | Qualify original source versus handoff, ownership reason, loose/worn/nested roots, spent materials and quill→sand→quill identity changes. Avoid making earlier history mandatory where the recipe permits supplied items |
| Indexed item plus capped/party experience | Native exact completion contract remains one outcome | Qualify frozen level/party/reward index, each item and XP entitlement/publication/save receipt, partial/busy/rejected/replayed/cold settlement and current custody. Do not promise nominal XP or count outputs as separate achievements |
| Disappearing giver is also a stock merchant | Accepted receipt remains distinct from recipient episode | Qualify remaining stock extraction, removal, absent-giver recovery, configured singleton reappearance and actual fresh-day supply. A daily candidate or retained receipt does not certify availability |
| Hidden consumable key and non-atomic access cost | Optional mask route explained, without an invented reward | Qualify chest reveal, exact carried/HOLD key, pre/post lock/open bits, admitted key-break result and mask GET separately. Handle interruption and another actor’s unlock; keep intentional100% break value |
| Computed service binding and inn properties | Actual teacher/trade limits and rent context are explained | Trace boot/table/flag binding alongside literal assignments. Qualify disabled payment services before enabling them; independently qualify native rent admission/save/failure. Static properties do not add achievement effects |
| Broad registry includes foreign assignments | Physical local dependencies reviewed with explicit boundary | Keep source ownership/registry-range/physical prototype filters distinguishable in tooling; improve evidence indexing without reassigning catalog owners or silently claiming foreign code coverage |
| Wording / custom story aspirations | Only proved sand selection defect ships | Later optional spelling pass may address “bakpack”, “Pheonix”, “Oakt” and request spelling without changing terms. Clarify hallway/forest direction intent before proposing topology edits. Builders must decide whether exams, spellcasting, factions or training need actual accepted effects; absent effects alone are not proof of a bug |

The actual sand repair is separate from these proposals. Already-issued secret
sand remediation and all runtime service/event expansions remain plans; no
operational write, account activation, migration or legacy economic bypass occurs.

## Validation and checkpoint boundary

Focused source fixtures retain all three exact contracts/D flags, local source
types/caps/parents, hidden chest/key/intestine intent,100% key break, closed
doors, computed teacher, actual inn flags and broad registry evidence scope.
Existing C++ projection journeys exercise the complete schema/file-loader,
discovery and contact visibility, worn/wrong inputs, supplied sand skipping
student history, spent quill/sand, read-only rendering, three independent
outcomes, immutable owner, replay and cold recovery. Native selection regression
uses actual production functions and original-fails/corrected-passes evidence.

Maintained build, changed/staged formatting, source/catalog tests, whitespace,
native/prior-map/catalog preservation and local source links are checked at
publication. These are local source/projection proofs: actual original-source/
corpse/handoff acquisition, SEARCH/key/GET/OPEN, offering/item+XP settlement,
merchant/student extraction and reset reappearance, teaching/trading/rent,
persistence and daily renewal still need accounting-active gameplay journeys.

Catalog89 maps/1615 achievement units/1459 potential dailies/2207 projected
rows. All2668 native definitions/fingerprint/revision two/registry and the
prior88 journals stay unchanged. Original queue68/220 comprehensive,152
pending; Myrabolus (`mira`) is next. This is a source checkpoint, without a
claim that the full220-zone goal is finished. PR remains based on
`experimental-accounting`; no DB/server operation, deployment or merge.

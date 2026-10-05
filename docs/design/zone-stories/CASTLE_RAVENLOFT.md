# Castle Ravenloft: temporal essences, documents, relics and the bell

Priority104 of the original220-zone queue. This is a **source-comprehensive map**;
played accounting/source/access/persistence qualification remains pending. The
[schema3/revision1 journal](../../../areas/story/ravenloft.story.json) maps four
independent stories, five contacts/42 verified aliases, six optional current
materials and one same-item prop exclusion. All five native accepted receipts
remain. Only Perganan is a potential local daily; discovery stays separate.
New discovery/encounter/journal/achievement/daily credit requires active, ready
accounting; frozen recovery remains separate.

## Source closure

| Source | Complete review and implication |
| --- | --- |
| [Quests](../../../areas/qst/ravenloft.qst) | All5 QA,17 addressed M/MA and10 ambient blocks, complete native text, item bundles/rewards and departures. Addressed aliases reveal contacts; ambient qc_action and each keyword do not manufacture achievements. |
| [Rooms](../../../areas/wld/ravenloft.wld) | All269 rooms58300–58568,173 full prose families,49 headers,5 metadata families,689 exact exits/219 relative patterns/80 complete exit texts. Registry57696–58568/mode0. Chapel/office/jail/crawlspace, courts/towers/garden/treasuries, past Abyss/future Zionyn, Ferrik battle/mines, cellar/catacomb routes and all falling/extra plaque records read. |
| [Mobiles](../../../areas/mob/ravenloft.mob) | All89 full records58300–58388:89 prose plus paired numeric metadata. Independent Obox-Ob forms and Strahd placement, adventurers/lost souls/Vistani/guardians/familiars and two unnamed prototypes preserved; imported nightmare12401 and outer Megosh58846 read. |
| [Objects](../../../areas/obj/ravenloft.obj) | All140 full records58300–58439, flags/values/traps/applies/inscriptions. Two documents, two essences, two holy relics, mallet/bell/cases, fixed controls, keys/currency/equipment/scenery/travel/books; all15 imported prototypes read. Quest readiness is distinct from actual source acquisition. |
| [Resets](../../../areas/zon/ravenloft.zon) | All937 declarations:D110/O50/P42/M316/E191/G78/F150;462 exact/538 parent-aware signatures/384 complete expanded argument/location/parent families. Caps/chances/parents/source containers and remote controls retained. Mode0 and one-shot contacts do not imply renewed supply. |
| Custom and shared execution | Entire [ravenloft.c/header](../../../src/specs/specs.ravenloft.c), all four active assignments, commented inactive strahd_charm, full shout_and_hunt, periodic setup/dispatch, bell command/special/object dispatch and synchronous extract_obj read. Exact existing kill achievement entry/transitions/kill_gain participants and disabled do_descend reviewed. Randomeq's highdrop list names two bosses, but its bonus lookup is commented out; ordinary drop calculation is not a guaranteed quest source. Unchanged shared quest, custody, switches/key/speech/door decoding, arrival and journal projection closure retained. |
| Foreign ownership | Global touching recipe/reset/room/portal scan read:5 local+4 foreign recipes,36 full reset groups,9 boundary edges across six full foreign rooms; four local incoming type25 declarations plus foreign handprint59325. All15 local imported object/two imported mobile records and foreign recipe rewards/keys/handprint reviewed. Reused unrelated coins/gems/books/guards in36 groups are source declarations, not Castle quest chains. [Catacombs](THE_RAVENLOFT_CATACOMBS.md) and [Barovia](THE_REALM_OF_BAROVIA.md) dossiers retain their complete owning-zone review unchanged. Barovia Continued's two outer-key recipes and Urik handoff reviewed here; its full review is next. |

## Exact native outcomes

| Quest | Exact native offering → reward | Local endpoint |
| --- | --- | --- |
| Lady Vey Rallen, QA18 | I58393+I58407 → I58408 | D1; independent story |
| a shocker lizard, QA29 | I58427 → I58427 | D0; excluded same-item prop |
| Perganan the rogue Vistani, QA88 | I58370 → I58416 | D0; independent story |
| the sabbath Wizard, QA137 | I58369 → I58401 | D1; independent story |
| Megosh, the Thief, QA162 | I58346+I58410 → I58411 | D1; independent story |

Lady Vey Rallen58312 starts at58324 and accepts both loose essences together.
Past Obox-Ob58364@58424 carries58393; future Obox-Ob58379@58560 carries58407.
Their independent appearances are not a spawn chain. Exact supplied stock fits
without personal prior kills, time travel, password knowledge or source custody.

Lenience58370 and Indulgence58369 start in desk58340@58321, with other papers and
scrolls. Lucian58348 carries box58366 holding key58362; the desk starts closeable,
closed and locked (15). Another copy of the desk at58383 has different contents.
Perganan58347@58320 accepts Lenience→note58416/D0; native text promises escape
but has no departure or escort. Sir Urik58804/Barovia Continued QA41 separately
accepts the note→Moonfriend58849/D1. Wizard58367@58544 has50-percent initial
placement chance and accepts Indulgence→belt58401/D1. No mallet is awarded.

Inner Megosh58381@58459 accepts Idol58346 (Ocap1@58317, trap retained) and
Dayheart58410 (Ocap1@58517) together→raven-talon key58411/D1. Its red court
door58459N→58369 uses58411; the reverse door has key0, preserving the asymmetry.
His narration mentions becoming Strahd but performs no actual spawn/transform.
Strahd58383 independently starts58562. The outer Megosh58846@58567 belongs to
Barovia Continued:devil heart58834→outer key58826/D1, separate from inner relics.
Madam Eva's independent locket58817+cloudy eye58824→holy symbol58825+outer
key58826 is the other route. The holy symbol and black gate key are distinct.

## Bell and shared sword case

Mallet58427 is Ocap1 in true Treasury58449. False Treasury gates use58421 from
a chelicera source; the true Treasury key58415 is nested in Strahd's keyring58420.
The Treasury also stocks tome58424/monolith360; these do not supply accepted
bell progress. Fixed bell58413 is Ocap1@58534. Native `HIT bell` requires exact
mallet58427 in WIELD and its worn bit; same-room floor special dispatch makes
the bell reachable. It searches remote58564 for the first case58430, narrates,
destroys the wielded mallet with legacy extract_obj, clears CONT_LOCKED and
rewrites case descriptions. It returns TRUE and bypasses ordinary combat HIT.

The case remains58430: its initial29 becomes21, still CLOSED/PICKPROOF.
Bright case prototypes58428/58429 are not selected or spawned. Case58430 Ocap1
holds imported25745, named Sunblade, longsword of Justice; mismatched Sunlash
whip descriptions require shared Bahamut builder intent. The tapestry58302
PUSH→58397N clears the blocked alcove approach; the bell changes container state,
not that exit. Missing case fails without consuming the tool; an already-unlocked
case still matches and can consume another tool. No personal solve receipt,
atomic accounting admission or durable remote-state/tool settlement exists in
this special. Adding credit requires that transaction, not a command observer.

Lizard58343 follows Cristofor58319 from58402 and accepts/returns58427 with a
squeak. This prop is excluded from the four stories, preserving its native ID
and accepted history. It neither produces the first mallet nor gates the bell.

## Access, exploration and cross-zone continuations

Outer58567S↔58568N uses58826; approach58958 is Barovia Continued. Other incoming
routes include Barovia91167DOWN→58568, realm14208SE→58369 and Catacombs59398E
→58305. They are legitimate declared alternatives, not mandatory personal proof.
Courtyard58412N uses58408. SouthTower58400S uses key−2/keyword obox-ob; SAY
unlock/reveal does not alone open the remaining closed door. Jail58315, slime
key58390, desk58362 and treasury keys govern different approaches.

Fixed switches preserve exact commands: tapestry58302 PUSH→58397N,58307 PUSH
→58442W,58309 PUSH→58440W; ophidian58306 RUB→58446DOWN; golden yuan-ti58305
RUB→58449NE; coin pile58422 GET→58563DOWN; brick58359 PULL→58457E; torch58349
PULL→58521N; altar58404 BLEED→58556DOWN; bone throne58406 PUSH→58560SE.
They change shared exit state and can be used independently of accepted hand-ins.
Past58303 NORTHWEST→58413 and future58312 NORTHWEST→58407 travel controls
start at58411 and58406 respectively. Ordinary return routes58425DOWN→58410 and
58561DOWN→58405 remain. Ferrik book58300 READ→58325 and return prototype58304
WEST→58426 are unplaced by the scanned active declarations; do not add reset
stock to repair an assumed route. Keep actual traps/sectors and F65@58504,
F90@58515,F100@58559/58563; prose does not override movement requirements.

Gertruda58412 O@58527→Mad Mary91052/Barovia QA638→91065/D1; this physical
item hand-in is not an escort event. Dracolich skull58397 O@58532→Rahadin59070
/Catacombs QA1704→Fortune59202/D0, independent of Strahd's skull. Old key58419
P in Strahd's keyring58420→Gevin59090/QA2315→spectral key59288+returned58419/D1.
Catacomb58538DOWN→59057 and58542S→59352 are separate entrances; key58419
continues beyond59352. Paired handprints59326@58456 PUSH→59065 and foreign
59325@59065 PUSH→58456 are type25 travel, not an exchange or proof of a solve.
Prison portal7371@58368 is another foreign route, not required local completion.

Native You Strahd Me at Hello progresses Doru91031→Chernovog58835→Strahd58383
and grants1000 epics. update_achievements accepts solo killer and eligible in-room
group participants through kill_gain. Journal integration must qualify ordered
actor death/participation and settled epic publication without duplicating native
credit. Bram's periodic Vistani help uses guarded shout_and_hunt with58382 helpers;
combat/no-silent/awake/path/distance checks remain, not a broken quest response.
Tome58424 applies timed equipped shadow shield outside belt attachment slots;
shimmer sword58400 has timed curse/blur and removal behavior. These item effects
are not quest endpoints. do_descend unconditionally returns disabled before its
old tome/book/five-orb consume and character transformation; do not revive it.

## Builder follow-ups and fair repair treatment

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-RAVENLOFT-BELL-TRANSACTION | `ravenloft_bell` requires the exact wielded mallet58427, CMD_HIT and bell argument; resolves the first case58430 in remote58564, narrates, destroys the mallet synchronously, unlocks and changes descriptions. Add one admitted atomic transaction for exact actor/tool UID/custody/generation and remote case UID/generation/state transition, with rejection, replay and cold recovery. Do not award a solve from a command, shared-open case or supplied sword. |
| ZSQ-RAVENLOFT-BELL-REPEAT-INTENT | Current code checks the case prototype, not its locked state. Because unlocking leaves prototype58430, a repeated strike can destroy another mallet after the case is already unlocked. Builder should decide intended repeat behavior; proposed repair is a guarded initial-lock/state predicate in a separately named fix/news commit, qualified together with accounting consumption. No repair applied here. Case starts29, becomes21: still closed/pickproof; do not replace it with unused bright-case prototypes. |
| ZSQ-RAVENLOFT-TEMPORAL-SOURCE | Past Obox-Ob58364@58424 gives58393; future Obox-Ob58379@58560 gives58407. Track successful first acquisition from the admitted source/corpse root UID/generation separately from player-supplied custody. Independent appearances and supplied essences require no personal time-travel/kill order. Qualify two-source bundles, transfer, nested/worn/spent stock and scarce reset admission. |
| ZSQ-RAVENLOFT-ACCESS-CONTROLS | Outer black key58826 is distinct from holy symbol58825; courtyard58408, count58411, jail58315, desk58362, false treasury58421 and true treasury58415 affect different approaches. Native key−2 speech door uses obox-ob. Fixed command switches and remote/shared exits need successful exact actor/control/door-generation transitions before personal solve credit; ordinary use of an open route remains valid. |
| ZSQ-RAVENLOFT-RECIPIENT-RENEWAL | Mode0; ghost/wizard/inner Megosh D1 retire, while Perganan D0 remains. Wizard M has50-percent initial chance. Only Perganan is a potential local daily; availability is not promised. Preserve contacts, caps and accounting-active source/factory guards until admitted stock/recipient renewal and recovery are qualified. |
| ZSQ-RAVENLOFT-CROSS-ZONE-RECEIPTS | Perganan note→Urik/Barovia Continued; Gertruda→Mad Mary/Barovia; dracolich skull→Rahadin and old key→Gevin/Catacombs. Local carrying/delivery, onward acceptance and returned-key custody are separate; freeze recipient/source identity and attempt ownership. Outer and inner Megosh are distinct independently placed actors; do not infer a transformation/spawn chain. |
| ZSQ-RAVENLOFT-KILL-ACHIEVEMENT | Existing update_achievements/kill_gain advances Doru91031→Chernovog58835→Strahd58383 and grants EPIC_STRAHDME1000. Solo and eligible in-room group credit differ. Define settled actor/participant death endpoints and idempotent epic/progress publication before adding journal stages; retain native ordering, accounting and party policy. No new reward or duplicate kill achievement here. |
| ZSQ-RAVENLOFT-TRAVEL-AND-ORPHANS | Placed past/future type25 controls, paired imported handprint59326→59065 and foreign59325→58456, plus prison portal7371→7497, need accepted arrival/custody qualification. Ferrik book58300 READ→58325 and return control58304 WEST→58426 have prototypes but no active local/foreign reset or touching Q placement found; other ordinary routes exist. Builder should decide intended placement/retirement before introducing stock or personal objectives. |
| ZSQ-RAVENLOFT-NARRATIVE-AND-HAZARDS | Megosh's transformation, Perganan's escape, Dayheart's sunlight lore, lost servants, garden, adventurers, Ferrik/Mustakrakish books and Vistani combat help are not automatic quest endpoints. Preserve actual traps, underwater/space sectors, falling65/90/100, combat helpers, item effects and supplied alternatives. Perganan exceeds the32-alias presentation limit: advertise one verified alias per response before synonyms, retaining native recognition; plan topic groups/pagination if builders need every synonym displayed. Define deliberate lore/exploration outcomes after played qualification. |
| ZSQ-RAVENLOFT-CAPTION-INTENT | Catacomb plaque58343 still says coming soon despite connected Catacombs; case loot25745 is named Sunblade/longsword while its long/extra descriptions describe Sunlash/whip. Proposed separate builder-approved caption/lore fixes require checking shared Bahamut ownership and intended weapon, original-fails/repaired-passes fixtures and prominent fix/news commits. No content repair selected or applied. |
| ZSQ-RAVENLOFT-DISABLED-DESCENT | do_descend returns a disabled message before its old tome58424/book500032/five-orb400231 consumption and race/class/home transformation. Do not present it as an available quest or restore it during mapping. Any intentional revival is a separate accounting-qualified character/item transaction with explicit builder scope; tome's current equipped shadow-shield effect remains separate. |
| ZSQ-RAVENLOFT-PROP-HISTORY | Exclude the same-mallet lizard prop from four story units while retaining all five native IDs/accepted receipts. It was already not daily-eligible. Qualify raw5→authored4 history, replay/cold recovery and separate discovery. The prop neither supplies an initial mallet nor proves a bell solve. |

No native content, quest, special, access, take flag or reward repair is applied
here. Potential repeated bell consumption and stale/inconsistent captions need
builder intent, accounting qualification and separately named fix/news commits.
Unplaced prototypes, ambient lore, mode0 scarcity and independent forms alone
do not establish bugs. The existing schema safely expresses contacts, four exact
hand-ins, current materials, supplied alternatives and cross-zone guidance; new
source, puzzle, escort, combat, travel or lore milestones require settled endpoints.

## Verification and limitations

Focused source/schema regressions pin all contracts and classifications, local
record/reset/source/control/case/boundary facts and active/inactive special paths.
Compiled journal journeys qualify independent two-item and document readiness,
wrong/worn/held/reward-only/spent stock, supplied bundles without source/access
history, all four accepted deliveries, excluded prop history, raw5→authored4,
replay and cold recovery. Full production catalog/inventory/audit regression,
maintained server build, changed/staged format and exact prior-map/native/queue/
PR preservation are required before publication. Synthetic accepted receipts
qualify projection rather than played source issuance, native consumption, remote
puzzles or operational accounting settlement. Full roadmap goal remains active.

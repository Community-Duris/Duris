# Zalkapfaan: comprehensive zone story dossier

Priority 169 in the original 220-area order. The [builder mapping](../../../areas/story/headless.story.json) maps eight exact native returns into eight cards across four progression families: engineering, bounty, armor requisitions and Malra-Kaan token paths. Nine contacts expose two addressed dialogue aliases. Twelve optional current-custody rows and eight native receipt steps preserve five daily-eligible deliveries and three departing story-only returns. New progress requires active READY accounting.

## Native progression and consumed inputs

**Engineering.** Engineer2749 Q11/M204@2746 accepts assembly2778 for earring2782 without D. M topic project blames the quartermaster; the actual source is Zaal-kab2786 M333@2785/G335 assembly2778. Bill of lading2761 mentions the delivery but is not accepted by this return. The promised war-engine immunity is dialogue, with no selected engine/faction outcome. Supplied exact assembly fits.

**Ear bounty.** Officer2755 Q25/M240@2756 accepts ears2735 for circlet2786 without D. O147 battered elf corpse2733@2750/P148 ears2735 is the exact source. The container's represented corpse is not proof of the player's own kill or an actual player-death settlement. Recover or receive the ears, then record the accepted exchange.

**Armor requisitions.** Smith2758 M237@2755 has Q34 requisition2764+xorn scales2771→vest2783; Q42 requisition2764+serpent scales2765→armor2784; Q50 requisition2764+queen scales2777→armor2785. All are independent, nondeparting item returns. Requisition2764 is G235 on tired soldier2762@2753; xorn scales2771 G298 on hatchling2773@2779; serpent scales2765 G275 on sea serpent2766@2770; queen scales2777 G332 on queen2785 M331@2784 (declared chance33). Reused vnum types do not make consumed roots reusable. Dialogue hours do not encode a timer, deposit, fee or exclusive choice; _noquest_ on reward2785 does not remove the explicit native contract.

**Religious tokens.** Deacon2780@2765 E260 wears Malra token2747; bishop2779@2763 E257 wears Kaan token2748. Chrome golem2777 Q59@2765 accepts2747→mithril key2750/D1; blood golem2778 Q73@2763 accepts2748→iron key2749/D1. Religious advisor Zekrallin2787 Q93/M338@2786 accepts both tokens→shattered soul2780/D1; M topic piety. Each departing giver is story-only under resetmode0. None of these receipts requires prior conversation, killing a golem, using a key, entering the temple or personally recovering a token. The same pair cannot also fund both golems without fresh roots. Item2778/2749 and actor2778/2749 are different typed identities.

## Gates, passwords and roaming availability

Both surface edges are reciprocal: entry2730N↔596968S and harbor2738S↔597768N. The directed union starts from actual local targets2730/2738: eligible OPEN40, SEARCH+OPEN40, eligible PICK40, all usable selected keys52, and keys plus admitted spoken passwords54 of60 rooms. This union assumes success and usable keys; it proves neither stock nor sight, survival, personal discovery or actual arrival. Six staging/holding rooms2784..2789 have no ordinary player incoming route.

Portcullis2731W is pickproof raw3/key2732; reverse2732E is pickable raw2, and both reset locked. One gate guard2754@2731/G158 carries virtue2732. The harbor approach reaches the city from the other side, so that key is not a universal prerequisite. Warden2768@2767/G264 copper2767 opens pickproof2768S↔2771N. Midnight guard2770@2776/G286 adamantium2773 opens two pickproof war-room approaches2774E↔2782W and2776N↔2782S. Held or loose keys qualify; nested keys do not. Virtue/copper value1=100 is100percent post-unlock break chance; iron/mithril/adamantium value1=0 has no native break chance.

Religious bastion2764N↔2763S uses iron2749;2764S↔2765N uses mithril2750. Study2780W↔2781E uses key=-2 and final keyword kal-zerr, written on badge2770. Bastion2764W↔temple2783E uses key=-2 and final keyword malra-kaan. [Word-key handler](../../../src/cmd/actcomm.c#L186) runs only through admitted [SAY](../../../src/cmd/actcomm.c#L496), clears reciprocal lock/secret bits and leaves OPEN/movement separate. Temple raw7 is masked to pickproof3 by [loader](../../../src/world/db.c#L1511); D reset2 on2764W/D1 reverse is not secret. Do not manufacture a SEARCH or saved-name gate from prose.

Queen2785@2784, Zaal-kab2786@2785, Serran2788@2785 and Zekrallin2787@2786 all lack SENTINEL and have STAY_ZONE. Ordinary [mundane roaming](../../../src/mob/mobact.c#L8020), when admitted and not fighting/following, can move through the staging chain to2788, whose ten exits lead to ordinary zone scenes; destination NO_MOB and other movement gates still matter. Diversion south into2789 ends at a closed locked blocked self-exit (D10). The room's1/16,1/8,1/4,1/2 text is not an implemented encounter probability. Queen M331 arg4=33 is admitted only on a nonzero forced reset under the current [M admission](../../../src/world/db.c#L3584); admitted row then rolls33percent. Initial normal boot usesforce2, while copyover/recovery skips newspawn. Review intent before changing this availability.

## Optional services and scene outcomes

The full selected compiled-reference closure includes the Commodore2733 teacher table, imported charisma pool67, epic stone359 and generic mine193. Memory55189 has no selected special or native return. Local ash2670 appears in alchemist issuance code but is outside this area's physical objects and reset imports; that numeric overlap is not a local binding. There are no local literal mobile/object/room specials, disabled local bindings, shops, incoming object portals or foreign recipes/exported source resets.

Commodore2733@2736 is a table-driven [ship damage control teacher](../../../src/classes/epic_skills.c#L218), installed by epic initialization. [PRACTICE handler](../../../src/classes/epic_skills.c#L453) uses the full skill name, configured level/cost and learned-state eligibility; [active accounting guard](../../../src/classes/epic_skills.c#L684) declines new purchases. Its existing callback charges coins after epic debit, checks expected skill, updates learned/taught and saves; this is a follow-up accounting service, not an active journal completion.

Broken earth193 O146@2747 is bound by [mining initialization](../../../src/economy/mining.c#L598); the [mine handler](../../../src/economy/mining.c#L273) declines new mining during active accounting before pick/skill/timed output. The quarry therefore cannot become a required current story step. Smith armor exchanges consume scales and are independent of mining.

Pool67 O149@2781 uses [charisma wrapper](../../../src/specs/specs.heavens.c#L1095)/[common handler](../../../src/specs/specs.heavens.c#L855): PC DRINK, level51, shared48hour TAG_POOL, early reuse injury, conditional full healing and a random bounded stat change that may be neutral or negative. Do not award a positive-gain objective for possession or mere command dispatch.

Commander2730@2781 G309 carries epic stone359 and G310 memory55189. The [node handler](../../../src/world/epic.c#L1111) sets zone identity periodically, selects the exact visible loose/floor stone for TOUCH, requires peaceful same-zone/level eligibility, freezes participants and submits a durable award. Committed [zone touch publication](../../../src/world/epic.c#L1308) can request mode0 reset. Possession, node lore, memory loot and accepted native return are separate outcomes.

Guillotine2731 O150@2783, prisoner2737, cardinal2781, head2782, animated headless2783 and executioner2784 form a temple scene without selected conversion, execution, liberation or soul-release terminal. Ships in dock prose are ordinary local room scenes, not an inferred shipping-service contract. All57 local objects, including extras/affects, retain ordinary loot versus native-input distinction.

Two moat rooms use actual water-swim6;2769 has C15west. Current [command](../../../src/cmd/interp.c#L1908), [room entry](../../../src/world/handler.c#L1516) and [movement](../../../src/cmd/actmove.c#L1401) paths depend on water, flight/levitation, successful direction and admission. There is no local fall metadata. Raw mobile1d1+1 is augmented by [level-squared loader HP](../../../src/world/db.c#L2274), so no combat rebalance is inferred from raw dice.

## Reset and accounting boundaries

Resetmode0 omits ordinary [periodic boot scheduling](../../../src/world/new_events.c#L1983). A committed node touch may schedule a reset check, and [mode0 handler](../../../src/world/db.c#L3222) uses epic completion plus DB reset percentage before an actual reset. This conditional path is not guaranteed daily replenishment. [Active accounting guard](../../../src/world/db.c#L3333) rejects O/P/G/E item issuance before read_object, including materials, worn tokens, nested ears, pool and node stock. Preserve that boundary; a durable reset-generation/source identity is required before new stock publication.

No native code or world-data repair is selected here. Review the engineer source/promise, armor delay wording, holding-room fractions, queen reset admission and optional service accounting with builders. Any selected native repair needs a separately named fix/news commit, exact before/after tests and prominent PR/news wording.

## Source closure and validation

Complete selected native closure:60 rooms/132 exits/59 mobiles/57 objects/227 commands/full340 ZON/108 QST. All60 room prose,10 header,3 metadata,52 exit-pattern and7 exit-text families;59 numeric/prose mobiles;57 complete objects;144 expanded reset families;4 imported objects;0 imported mobiles;0 foreign reset families/recipes;4 boundary edges/2 complete outside rooms;51 typed compiled rows and selected generic handlers are qualified. No missing prototypes, reset rooms, walking exits or shops. Wider service economy and ownership episodes remain separate.

Passed publication gates: full production catalog/inventory/audit regression, all182 compiled Python/C++ journal journeys, meaningful source/schema assertions, changed/staged canonical format and exact scope/native/prior-map/roadmap/link/PR preservation. The maintained Linux server build passed; src is unchanged here. The unchanged full production regression passed on an isolated native Linux filesystem with all6197 selected input files verified by SHA-256 against the worktree before recording the result. All182 compiled journal journeys ran successfully in the worktree on Windows with the maintained native compiler and static cJSON dependency. No played active-accounting engineer or bounty hand-in, armor delivery, religious return, spoken door, staged encounter, pool effect, node touch, mining, training or persistence outcome is claimed.

## Follow-ups and builder decisions

1. **Consumed branches.** Represent consumed-root alternatives for three armor requests and the two golems versus Zekrallin. Preserve all eight independent receipts and five-daily/three-story-only classification. Do not impose an exclusive choice or a quest order that native contracts do not require.

2. **Source versus supplied materials.** Record item UID, source actor/container, original spawn episode and transfer cause for assembly, ears, requisition, scales and tokens. Distinguish gifts, theft, trade, corpse recovery, worn token removal, nested transfer and reacquisition. Preserve supplied exact-item acceptance.

3. **Durable native returns.** Freeze exact giver/binding, admitted visible owned loose roots, recipient eligibility and item rewards before publication. Test all eight receipts, duplicate delivery, departure cleanup, replay, absent actor and cold recovery. A receipt does not prove personal source work or newly obtained rewards.

4. **Armor assembly.** Show each exact two-input requirement with missing/current counts and its independent receipt. Native dialogue mentions hours, but no wait, partial deposit or additional fee is encoded. Any future asynchronous crafting job needs its own accepted state, cancellation/recovery and delivered outcome.

5. **Conversation knowledge.** Record addressed project and piety responses separately from SAY, alias recognition and a saved knowledge fact. Golem reward text reveals names; it does not automatically produce an owned password objective. Keep conversations optional until builders deliberately encode a gate.

6. **Spoken access.** Publish admitted successful word-key unlocks with actor, exact exit pair, keyword identity and before/after lock state. Follow with owned OPEN and actual arrival. Test wrong words, unadmitted speech, repeated unlock, reciprocal state, reset/relock and recovery; do not count raw speech.

7. **Physical key access.** Track current held/loose keys2732/2749/2750/2767/2773 and actual unlock/open/arrival. Virtue and copper keys break100percent after successful unlock; others have zero native break chance. Nested custody and historic ownership do not establish usable access.

8. **Conditional routes.** Keep two actual entry targets and the directed40/52/54 access scenarios explicit. Key branches and spoken-password branches differ; shared alternate approaches can avoid a gate. Do not require all religious returns to reach a scene or award progress from static reachability.

9. **Staged roaming actors.** Qualify queen/advisor/religious-advisor initial stock, movement through2784..2788, actual encounter, holding-room diversion and death. Existing non-sentinel STAY_ZONE actors can roam into ten scenes; six rooms without player incoming paths alone do not prove broken quests.

10. **Declared chance and reset admission.** Review queen M331 arg4=33 against current M admission requiring arg4==100 on ordinary resets unless forced. Initial boot force2 can admit the33percent roll. Prose1/16..1/2 is not the reset probability. Establish intended behavior with builders before selecting a named native fix.

11. **Dynamic availability.** Explain absent givers, departed story-only actors, consumed materials, holding-room stock, copied world recovery and pending authority. Mode0 omits ordinary timer scheduling but has a conditional post-node-touch DB reset path. Daily rollover neither issues stock nor guarantees a meeting.

12. **Accounting reset issuance.** Add a durable reset-generation identity and atomically publish admitted O/P/G/E equipment, pools, earth, nested ears and node/memory stock with source lineage. Existing active accounting guard declines item issuance before read_object; never count a declared reset as a successful acquisition.

13. **Charisma pool outcomes.** Record exact selected successful DRINK, level51 eligibility, shared48hour TAG_POOL, prior stat/health, frozen RNG, bounded result and persisted effect. Separate cooldown injury, healing, neutral or negative stat outcomes and positive gain; possession is no pool completion.

14. **Owned node touch.** Qualify commander-carried stone359 periodic zone identity, recovery to visible loose/floor custody, peaceful eligible TOUCH, exact UID and committed group award/reset request. Memory55189 is a separate souvenir. G issuance, possession and command text do not prove a committed node outcome.

15. **Epic teacher accounting.** The Commodore is bound through the epic teacher table, but current active accounting declines purchases. Add an atomic epic/coin payment plus frozen skill state and persisted learned outcome, dynamic availability and rollback/recovery. Do not bypass the current guard or invent class restrictions.

16. **Mining accounting.** Broken earth193 is generically bound to mine, which currently declines during active accounting. Add frozen resource depletion, pick/skill eligibility, timed work, output item UID/quality and cancellation/recovery publication before any counted quarry stage. Keep mining separate from scale delivery.

17. **Temple acts.** Guillotine, prisoner, cardinal, severed head and newly animated headless are lore/combat scenes without a selected execution, conversion, rescue or soul-release terminal. Builders must design participant eligibility, victim ownership, reversible failure and saved outcomes before a story hook.

18. **Water and combat outcomes.** Moat2769 has current15west in actual water-swim6; flight/levitation, command eligibility and successful movement matter. Record owned arrival/survival only after resolution. Mobile1d1+1 is augmented by level-squared HP in the loader; do not infer a combat repair from raw dice.

19. **Lore and builder decisions.** Review engineer quartermaster blame versus advisor source, promise of engine immunity, armor work duration and obsolete holding fractions. Current mappings state the encoded behavior fairly. Selected data/quest repairs need separate named fix/news commits, precise before/after and prominent PR/news notes.

20. **Played qualification.** Run authorized active READY accounting journeys for all five daily and three story-only exchanges, gifts/source recovery, repeated/competing inputs, departure, passwords/keys, staged encounters and recovery. Qualify pool/node outcomes separately; teacher/mining guards remain visible. Source and compiled regressions do not replace played proof.

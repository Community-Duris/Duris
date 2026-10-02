# Bastine Castle: comprehensive source story map

Reviewed October 2, 2026. Source area `bastine`, zone 76, journal revision 2.
Reviewed all 129 local rooms, 39 mobile prototypes, 80 objects, 428 resets,
14 Q contracts and four addressable M responses. All fourteen native exchanges
remain independent achievements: twelve royal commissions, the werewolf-prince
request and Victor's trust exchange. No contract is excluded.

## Evidence boundary

Reviewed the complete active [Q/M source](../../../areas/qst/bastine.qst),
[rooms](../../../areas/wld/bastine.wld), [mobiles](../../../areas/mob/bastine.mob),
[objects](../../../areas/obj/bastine.obj) and [resets](../../../areas/zon/bastine.zon).
There is no local shop file or local special found in the literal assignment
and named-procedure scans. The [generated audit index](../../reference/zone-story-audits/bastine.md)
captures exact bindings, response lines, reset totals and prototype ownership.
Shared quest, lock/container and reset execution remain part of the review;
absence of an assigned local procedure does not make room/object data inert.

Inspected the exact external trophy sources in Southern Coastal Highway,
Forest of Mir, Breale, Skulldrach and Test of Knights, plus the complete
Morlanthra continuation: its Q/M block, rooms, four keys, named captors and
source resets. These bounded dependency reads do not qualify all quests,
specials or access routes in those other zones. Computed special assignments
remain a manual extractor limitation.

## Progression: the King's road to knighthood

King Mrotha 7600 starts at throne 7626. `hi`/`hello`/`knight` offer service,
`no` dismisses the offer, and `yes` narrates the Initiate rank and first task.
None saves rank, accepted membership or a mutually exclusive branch. The
source then describes twelve promotions in the order below. Journal revision
2 follows that source order rather than sorting the item IDs.

Every offering is one exact item; every listed fee is zero. Coin amounts
are **rewards in copper**, with 100 copper per gold. Q matching requires the
trophy, not earlier promotions, conversations, personal kills, group kills,
a previously worn insignia or a saved rank. Commission titles describe
the King's narration, not enforced rank state.

| Narrated result | Accepted item | Confirmed reset source | Reward / next commission |
| --- | --- | --- | --- |
| Young Warrior | Intimidator dirk 41388 | Thief captain 41351, Highway room 41787 | 2,000 copper; Vinzor's crown |
| Warrior | Iron crown of King Vinzor 41407 | Loose O source at Highway room 41683 with King Viznor 41303 | 10,000 copper; blessed shield |
| Adept Warrior | Blessed shield of defense 12802 | Stone golem 12704, Skulldrach room 12796 | 5,000 copper; anti-orc scimitar |
| Veteran | Anti-orc scimitar 41327 | Bishop 41327, Highway room 41672 | 15,000 copper; ebony staff |
| Soldier | Ebony staff 41924 | Johan 41908, Forest of Mir room 41939 | 50,000 copper; broom of air |
| Lieutenant | Broom of air 2607 | Pontif 2602, Breale room 2654 | 75,000 copper; Montel's mask |
| Third Ranked Captain | Cloth mask 41408, identified as Montel's | Sir Montel 41357, Highway room 41566 | 100,000 copper; Mythra's face |
| Second Ranked Captain | Face of Mythra 41922 | Mythra 41910, Forest of Mir room 41960 | 100,000 copper; Xort hide |
| First Ranked Captain | Xort hide strip 41920 | Xort beast 41909, Forest of Mir room 42032 | 50,000 copper; Black Knight's ankh |
| Captain | Ankh of the black sword 41375 | Black Knight 41343, Highway room 41810 | 100,000 copper; Kearonor hide |
| General | Original Kearonor hide 41411 | Kearonor Beast 41302, Highway room 41724 | 200,000 copper plus replacement hide 41304; Life artifact |
| Knight of the Bastine Order | **Wand of Life 70970** | Kithron 70941, Test of Knights room 70925 | 1,000,000 copper, order seal 7617 and platemail 7616 |

World ownership follows prototype files: 41920/41922/41924 belong to Forest
of Mir even though some VNUM/range guesses could suggest Highway. A reset
source is not proof of peaceful acquisition or a personally credited defeat.
The crown is a loose object, not a death-created trophy. A gifted trophy is
accepted by all twelve commissions.

The General exchange consumes hide 41411 and supplies **different prototype
41304**, with the same name/aliases and description. It adds `ITEM_NORENT`;
both prototypes already have `ITEM_NODROP`. Its prose calls this an obscure
curse. This is a replacement reward, not evidence of an in-place spell cast
or the same UID returned. Only 41411 is the required offering.

The source asks for the "Staff of Life" but accepts the prototype named
"the wand of Life". The journal names the actual accepted wand and explains
the King's wording. Likewise, the ankh commission's response calls the
offering a helm, and Mythra's response refers to a patch. These are content
discrepancies to review deliberately; do not silently change accepted items,
rewards, receipts or world balance.

The final commission shows all eleven earlier receipts as optional history.
A supplied wand can complete it independently. It does not complete those
eleven commissions or a new all-stage campaign. A future full knighthood
achievement can require an explicitly reviewed complete set while retaining
the independent native achievements. Do not reinterpret an any-of terminal
binding list as an all-stage dependency.

After the final reward the King proposes defeating the Firestrom Dragon
for the knight's own honor. No further local Q records proof or supplies
another royal reward. Retain this as a later adventure lead until an owned,
committed encounter objective and its intended reward policy are designed.

## Progression: the imprisoned prince and Phex's secret

The King's bedroom 7628 has an east passage to "Secret Chamber" 7629.
Both directions are reset closed/unlocked. The east room property has
door-state 5, but the loader masks it to its ordinary door bits and the
D reset uses state 1. It therefore does not establish a currently secret
exit. The chamber's Phex plea leads down the stairs into the Caverns of
the Lost King. This is discoverable lore rather than a saved examination
or password objective.

The caverns end at dusty room 7664. Non-takeable desk 7667 is closed but
unlocked, and its nested resets contain **tower key 7601** and **note 7668**.
The note tells Phex to feed the prisoner, guard the King's secret and use
the key if the prisoner becomes unruly. The skeleton and old papers here
are room description; no Phex quest giver is present in the reviewed mobiles.

The south guard quarters 7603 lead to tower stairs 7699 and the sequence
7717–7721. The up door at 7721 and its reciprocal at prison 7722 are locked
and pickproof, requiring tower key 7601. Prince Prisoner 7633 starts inside,
with **werewolf head 7673 as a G reset item**. The head is already his
inventory item; the Q does not create it on death or authenticate a kill,
beheading, infection or personal recovery.

Offer that head to the King for **werewolf-hide leggings 7674**. He narrates
grief, disease risk and a wish to be left alone. Q has no disappearance,
banishment timer, infection, cure or accepted-rank state. The leggings are
ordinary equipment with properties, not a recorded "cured lycanthropy"
objective. General combat/disease behavior needs its own live qualification
before the journal claims that effect. A supplied head is accepted without
the completer's key, clue or combat history.

Revision 2 gives the tower-key route as optional preparation. This request
remains independent of the royal commission sequence. Builders should decide
whether the bedroom passage ought to be secret; align room and reset state
only after confirming intent and preserving a usable route.

## Progression: Victor's trust and Morlanthra's rescue

Victor 7603 starts at Morlanthra's bedroom 7620, south of main hall 7619.
Its door is reset closed/unlocked. `hello`/`hi`/`morlanthra` request proof
of strength: **Lamerok's signet ring 41397**. It starts equipped on Sir
Lamerok 41355 at Highway cave 41701. Before identification its visible name
is a shiny signet ring; `_id_` descriptions provide the named identity.

Victor's exchange supplies **small steel key 41394** and **apprentice's ring
41398**. It does not remove him or release Morlanthra. His "one quarter"
statement describes four ordinary key targets, not a four-input quest recipe
or a special that tests ownership of all keys together:

| Lead / source | Actual locked target |
| --- | --- |
| Victor's small steel key 41394 | West door 41701 → Esterra's room 41702 |
| Esterra 41313 holds plain iron key 41396 | North secret/pickproof door 41693 → waterfall 41703. D reset 41693 uses state 6, so discovery precedes access. |
| Nynevena 41314 holds transparent key 41395 at 41704 | North magical curtain 41703 → pond 41705; locked/pickproof |
| Odd tree-shaped key 41343, loose O source in red-dragon lair 41540 | North tree door 41705 → Morlanthra's prison 41706; locked/pickproof |

Morlanthra's message is a room extra description at 41691; Esterra's order
to Nynevena is room text at 41704. These explain the imprisonment and are
clue candidates, not currently recorded examination results. Access can
also depend on an already opened route or valid keys supplied by others;
the final quest does not verify all four keys or their source histories.

Once inside, **Morlanthra 41315** accepts apprentice ring 41398 for **2,500
experience and amulet of petrified dragon eyes 41405**, then disappears.
His Q/M block belongs to [Highway](../../../areas/qst/highway.qst). The
native reward is an amulet, even though informal rescue prose may call it
a necklace. His dialogue accepts greeting/prison/free, proof and the
Volheru/king/Mrotha/Victor/knights topic family. That final family rebukes
the subject rather than authenticating a saved branch.

The Bastine journal explains continuation after Victor's exchange; it
does not annex Morlanthra's contract, count the rescue twice or make an
out-of-zone receipt a local required step. Future cross-zone campaigns
need explicit owners, linked local entries, reveal rules and recipient
policies. The generic fallback retains Morlanthra's own native receipt.

## Supporting lore and non-quest exploration

Court knights' biographies describe jousts, family feuds, Staff of Life
history and past feats; the training/jousting rooms describe activities.
No local Q or assigned procedure turns these into current tournament,
training, membership or duel objectives. The treasury, maids, nobles,
archers, moat trout/octopus, cavern creatures and animated armor have
ordinary encounter/equipment behavior. Neither clearing the moat nor
purging the caverns is required for any royal exchange.

Seal 7617 from the final commission is distinct from highest-seal ring
7635 equipped on Sir Lagarias. Matching order terminology does not make
looting the latter equivalent to earning the former. All named recipe
inputs, keys, notes, ordinary loot and differing short/identified names
were reviewed without adding hypothetical stories for every object.

## Blockers, proposed improvements and balanced repairs

| Finding | Evidence / impact | Planned action |
| --- | --- | --- |
| ZSQ-RESET-AUTHORITY | 18 D, 13 O, two P, 158 M, 200 E and 37 G local commands; external trophies/key sources span five areas. | Qualify active generation, exact reset occurrences, nested desk sources and foreign NPC equipment. Existing recovered objects are not proof of fresh reset support. |
| ZSQ-ALL-STAGES / NARRATED-RANK | Twelve promotion receipts are independent; invitation is dialogue only. | Add an explicitly all-stage campaign and accepted membership/rank rules if builders want them. Preserve gifts, native receipts and current independent commissions. |
| ZSQ-REUSED-REWARD-LINEAGE | General reward replaces 41411 with 41304; two hides look identical. | Record exact input/output UIDs and declared transformation. Explain actual differing prototype/property without inferring personal kill credit. |
| ZSQ-CROSS-ZONE-OWNERSHIP | Victor's local trust exchange starts Highway's four-lock/rescue route; room/type IDs alone give wrong ownership clues. | Add owned cross-zone links and NPC/source identity, with clues, access and terminal receipt distinct. Avoid double-counted completion/daily projection. |
| ZSQ-BASTINE-TERMS / CONCEALMENT | Staff/wand, helm/ankh, patch/face and bedroom secret/reset wording differ; Firestrom challenge lacks a local completion. | Builder reviews prose versus intended execution. Journal now names accepted items and real keys. Change contracts, secrecy or challenge goals only through a reviewed world-content revision. |
| ZSQ-BASTINE-PRINCE | Head is reset inventory; grief/infection/banishment text does not encode those states. | Qualify ordinary combat/gear effects separately. Choose narrated closure versus new committed source/encounter/state objective before claiming a cure or personal kill. |

## Qualification matrix

Under active accounting, qualify each of the twelve correct/wrong trophy
offerings, coin/item reward outcomes, gifted final wand without promotion
history, duplicate/repeated independent commissions, original versus replacement
hide, identified versus unidentified signet ring, tower-key nested source,
bedroom/passage concealment, prince head source versus gifted delivery,
Victor's outputs, every external key target and already accessible route,
Morlanthra's own giver/terminal disappearance, group recipients, interrupted
publication and two cold restarts. Do not award kill, rank or learned-clue
history from an item or a line of dialogue.

Focused native regression proves the supplied-final-wand next action,
independent commission completion, unchanged counts and receipt recovery.
The live external expedition, active reset generation, new cross-zone
campaign, source lineage and historical clue/access/encounter objectives
remain pending. This source-comprehensive map is not full gameplay qualification.

# The Motte and Bailey of Duke Delwyn: comprehensive source story map

**Source-comprehensive, revision one — October 3, 2026. Gameplay qualification
remains open.** The [sidecar](../../../areas/story/delwyn.story.json) explains
all eleven native exchanges as six independent outcomes and five paid services,
with nineteen contacts, all nine addressed dialogue families and seventeen
optional material/history checks. Six potential daily candidates remain subject
to material, recipient, episode and accounting readiness. **Actual native zone
or quest repairs in this checkpoint: none.** Proposed repairs are identified
separately below and are not player news claims.

Active, ready accounting is mandatory for discovery, encounters, journals and
new credit. Four paid textile stages and one coin-only encounter are guarded
until durable payment settlement is supported. Their historical accepted
receipts can explain a route without enabling new settlement, paying a fee or
awarding achievement credit. An exact supplied banner can satisfy the separate
item-only banner delivery without four personal crafting receipts.

## Evidence and review boundary

Reviewed all 53 [native blocks](../../../areas/qst/delwyn.qst): eleven Q and
42 M responses, comprising nine addressed families and 33 ambient `qc_action`
families. All contracts are classified, including the coin-only encounter.
Ambient inspection, tariffs, craft demonstrations, ale, guard movements,
servant activity and the lookout's false invasion alarm have no new quest
terminal. The [reproducible review index](../../reference/zone-story-audits/delwyn.md)
records the contracts, addressed families, materials and literal assignments. See the [static typed-dependency audit](../quest-story-followups/DELWYN_AUDIT_REVIEW.md) for cross-area ownership.

Complete world review covers all 207 [rooms](../../../areas/wld/delwyn.wld),
82800–83006: 170 exact prose groups, seven numeric headers, nine non-exit
metadata groups and 145 exit families. Reviewed all 96
[mobiles](../../../areas/mob/delwyn.mob), 41
[objects](../../../areas/obj/delwyn.obj) and 278
[resets](../../../areas/zon/delwyn.zon) across 170 source families. Commands
are 135 M, 67 E, 38 D, 28 O, five F, four G and one P. Every reset chance is
100 and every reserved argument is zero. Zone 828 resets in mode two; these
are declarations, rather than observed promises of repeatable supply.

There is no local shop file, no matching foreign shopkeeper declaration and
no literal local special assignment. Review included local identifiers/names,
automatic room/mobile binding, native quest dispatch, shared teacher,
container/trap, movement/falling and reset execution. The magician's
`ACT_TEACHER` flag binds the shared teacher in
[mobile loading](../../../src/world/db.c#L2764); its `level` advice is separate
from the flag topics in [teacher execution](../../../src/classes/epic_skills.c#L424).
No local custom shearing, chess game, flag-hanging, mill-state, spy allegiance,
scheduled invasion, cleric-visit or transformation terminal was found.

The ordinary boundary is [outside gate 82805](../../../areas/wld/delwyn.wld#L111)
south → [surface stone highway 549230](../../../areas/wld/surface.wld#L1033446),
with the highway's north exit returning to the gate. All inspected positive
boundary and reset targets resolve. The gate and interior door declarations
remain intact. Cross-check absence claims against the actual active source
before planning world repairs. No live travel is certified.

## Progression and exact contract matrix

Each Q binding remains owned by its actual recipient and exact normalized
recipe. All recipients stay after acceptance. Earlier producer receipts are
optional; the native contracts accept supplied exact materials. None requires
learned keywords, a personal kill or a campaign-wide branch.

| Recipient and native source | Exact offering → reward | Journal treatment |
| --- | --- | --- |
| [Clockmaker 82815](../../../areas/qst/delwyn.qst#L134) | Small iron bell 82828 → large iron cog 82829 | Independent outcome/potential daily; bell carried by bellfounder, no personal purchase or repair-state proof |
| [Miller 82808](../../../areas/qst/delwyn.qst#L74) | Large iron cog 82829 → spike-knuckled razor sharp blade 82830 | Independent outcome/potential daily; optional clockmaker receipt, no running-mill endpoint |
| [Spinner 82825](../../../areas/qst/delwyn.qst#L203) | Sheep fleece 82816 + 5000 copper → white yarn 82817 | Paid service; current material check cannot pay the fee |
| [Dyer 82821](../../../areas/qst/delwyn.qst#L160) | White yarn 82817 + 10000 copper → crimson yarn 82818 | Paid service; optional spinning receipt |
| [Weaver 82824](../../../areas/qst/delwyn.qst#L189) | Crimson yarn 82818 + 10000 copper → crimson fabric 82819 | Paid service; optional dyeing receipt, white yarn is a different input |
| [Seamstress 82823](../../../areas/qst/delwyn.qst#L174) | Crimson fabric 82819 + 35000 copper → crimson banner 82820 | Paid service; optional weaving receipt |
| [Magician 82895](../../../areas/qst/delwyn.qst#L356) | Crimson banner 82820 → velvet-lined lizard-skin slippers 82821 | Independent outcome/potential daily; optional sewing receipt, safe flagpole access separately required |
| [Off-duty captain 82874](../../../areas/qst/delwyn.qst#L276) | Carved ivory knight 82826 → spit-shined military boots 82827 | Independent outcome/potential daily; missing piece, without a played match or lesson |
| [Skulking halfling 82890](../../../areas/qst/delwyn.qst#L315) | Military assessment 82822 → hastily scribbled note 82824 | Independent outcome/potential daily; study desk source, without allegiance or siege credit |
| [Duke Delwyn 82878](../../../areas/qst/delwyn.qst#L285) | Scribbled note 82824 + deep blue braid of rank 82823 → signet ring 82825 | One two-item outcome/potential daily; optional halfling receipt, no personal kill or prevented invasion |
| [Pub alley contact 82807](../../../areas/qst/delwyn.qst#L54) | 1000 copper → no item | Paid service, without achievement/daily credit or a checked cleric visit |

The textile route costs **60000 copper** from fleece to banner. The four
services are visible as support, while the final return owns one outcome.
All four mixed offerings are `Unsupported durable offering` in native daily
metadata. The coin-only service is `No repeatable item offering`. These five
rows earn no authored achievement or daily credit; an existing accepted
receipt remains valid explanatory history. Item-only banner eligibility
does not make its guarded local producer route available with accounting
active. A supplied exact banner can still support a qualified item-only
delivery; source identity and ownership require separate evidence.

The bell route has two independent receipts. The clockmaker's accepted bell
produces the cog, and the miller consumes the cog for the blade. Clock and
mill scenery are not usable substitutes or stateful repair endpoints. The
captain's reply explicitly accepts the knight without caring how it was
found. His story does not require a chess victory or an en passant lesson.

The military-document route starts with assessment custody. The halfling
returns a note supposedly intended for an unnamed master. The same note
and his worn braid can then be delivered to the Duke. Both receipts are
valid independently: an earlier cooperation receipt does not prevent a
warning, and supplied proof does not require cooperation or killing the spy.
Native dialogue does not enforce exclusive allegiance, a timer, a master
identity, an invasion event or a defended-castle campaign.

## Sources, access and information discovery

| Material or recipient | Declared source/access | Qualification needed |
| --- | --- | --- |
| Fleece 82816 | [Shepherd 82852 and sheep follower 82853](../../../areas/zon/delwyn.zon#L210), declared at 82821; sheep carries one fleece | Active source, wandering follower location, acquisition and ownership; no local shearing/sale contract |
| Small iron bell 82828 | [Bellfounder 82833](../../../areas/zon/delwyn.zon#L265), workshop 82879; carries one bell | Active source and legitimate acquisition route; no purchase/commission acceptance |
| Cog, white/crimson yarn, fabric, banner and note | Exact outputs of their matrix recipes; no alternative active reset source found | Output lineage, supplied custody, mixed payment commits and spent-material handling |
| Military assessment 82822 | [One P reset](../../../areas/zon/delwyn.zon#L186) inside ebony desk 82814 at study 82965 | Container lineage, successful access/recovery, actor survival and personal first acquisition |
| Deep blue braid 82823 | [Skulking halfling equipment](../../../areas/zon/delwyn.zon#L353), one copy at study 82965 | Equipped versus root-carried material, acquisition, supplied evidence and ownership |
| Ivory knight 82826 | [Worried servant](../../../areas/zon/delwyn.zon#L379), one copy carried at recreation room 82980 | Active source and acquisition; no borrowing or match event |
| Banner recipient 82895 | [Magician reset](../../../areas/zon/delwyn.zon#L401) at flagpole top 83006 | Safe encounter, exact banner custody, accepted delivery and safe return |

The [desk prototype](../../../areas/obj/delwyn.obj) is a closeable, initially
closed container: values `300 5 0 300`; it is not locked. Its `T 512 4 1 30`
declares an opening trap, acid damage, one charge and level thirty, as decoded
by [object loading](../../../src/world/db.c#L2990) and
[trap flags/types](../../../src/combat/trap.c#L17). Ordinary opening clears
the closed flag before [checking the trap](../../../src/cmd/actmove.c#L2698).
The active [acid branch](../../../src/combat/trap.c#L658) can injure the actor;
the declaration is a real hazard, rather than proof that a quest tracker
knows the actor survived. Retrieving a document from a container, reading
its extra description and offering it are distinct events. The journal's
present-custody check requires an unequipped root item, not one left in the desk
or a carried bag.

The inner gate, keep entry, study, bedrooms and roof include closed doors;
all 38 D resets set state one, not locked state two. No positive local key
requirement was found. The outer gate uses a negative key marker, and the
roof description mentions a large lock, but prose alone does not override
the actual closed-only door/reset state. Room text also describes an
inaccessible defensive lever; no interactive lever/script binding implements
a bridge-cutting objective. Guard descriptions do not add a customs permit.

The ladder room 83003 has `F 5`, and midair rooms 83005/83006 have `F 80` and
`F 90` plus sector eight (`SECT_NO_GROUND`). They each have a real downward
exit. [World loading](../../../src/world/db.c#L1337) records the chance,
and [command dispatch](../../../src/cmd/interp.c#L1898) can initiate falling
before an attempted interaction. [Falling execution](../../../src/world/falling.c#L139)
checks flight, levitation, mount effects and an active climbing skill;
possession of slippers or a banner establishes none of those protections.
The magician's own levitation is not an effect conferred on the player.
Safe approach, successful offer and safe return need actor/route evidence.

Nineteen contacts preserve real command keywords. All nine addressed native
families are advertised after the relevant encounter. Duke, source animals,
bellfounder, servant, customs staff, hosteler and lookout have no manufactured
topic achievement. Ambient `qc_action` is excluded from advertised topics.
The magician's `level` teacher response is a separate shared service, without
a named zone-story terminal. Use [journal presentation guidance](../../guides/ZONE_STORY_BUILDING.md)
for discovered-zone orientation, encountered-contact visibility and concise
next-step display.

## Direct area-owner questions and conservative recommendations

| Question | Source evidence | Recommendation |
| --- | --- | --- |
| Which public title and command aliases should identify this area? | The normalized .zon title is “The Motte and Bailey of Duke Delwyn,” with source stem `delwyn`; no local command-target alias or competing owner is established in the dossier. | Use the current .zon title as the display name. Add any command alias only after checking its real binding and ownership. |
| Should the six item outcomes remain independent from the five paid services? | The sidecar maps eleven native exchanges to six outcomes and five services; only the banner delivery consumes a prior textile output as an item proof. | Preserve the separate exact receipts and service classification. Do not add a compulsory all-stages campaign. |
| Should supplied exact materials remain valid without earlier crafting or personal-source receipts? | Every native contract accepts its exact current proof; optional clockmaker, spinning, dyeing, weaving, sewing, and halfling histories explain sources but do not gate later offers. | Keep source history optional and preserve supplied-proof eligibility. Do not require personal crafting, theft, or an earlier receipt. |
| What is the intended future behavior of the four guarded textile services under active accounting? | Spinner, dyer, weaver, and seamstress accept exact materials plus 60,000 copper total across the route; all four are currently unsupported durable offerings and settlement is guarded. | Keep the guard until item, fee, output, and recovery can commit atomically. Do not charge or grant textile outputs from a readiness check. |
| Should the pub alley contact’s 1,000-copper encounter remain a non-quest service? | The contact accepts coins without returning an item; the native journal assigns no achievement or daily to this exchange. | Keep it outside story credit unless a bounded reward and accepted endpoint are deliberately authored. |
| Does the banner delivery require only an exact supplied banner and safe access, or should it prove personal textile production? | The magician accepts banner 82820 without a sewing-history requirement; his roof location has fall hazards, and a supplied banner is eligible. | Preserve the exact item-only delivery. Treat safe approach and return as separate route qualification, not proof that the actor paid or crafted the banner. |
| Should the halfling note and duke’s braid represent warning, cooperation, or an exclusive allegiance branch? | The halfling accepts a military assessment for a note; the Duke independently accepts the exact note and braid. Neither contract enforces allegiance, a personal spy kill, timer, or invasion. | Keep both receipts independently completable with supplied proofs. Do not infer a prevented invasion or exclusive faction choice from the handoffs. |
| Is an actual siege or invasion intended as a completed local story? | The lookout has a false invasion alarm and the Duke’s native contract does not check a siege, defended castle, or resolved threat. | Keep the alarm as ambient behavior unless a real invasion state and accepted resolution are designed. |
| Which advertised merchant, craft, lodging, or customs roles should have working local services? | The area has no local shop file or matching shopkeeper; descriptions mention occupations and lodging, but no price, stock, or inn binding is present. | Confirm only the services builders intend to operate, then define their actual keeper, goods, price, and persistence. Do not invent stock or fees from prose. |
| Should fleece, bell, and ivory knight sources support peaceful handover, or are supplied/combat sources intentional? | Sheep, bellfounder, and worried servant carry requested objects, but no local shearing, borrowing, sale, or commission handover is bound. | Preserve exact native acceptance. Add a peaceful source only after defining limited issuance, ownership transfer, and accounting for any fee. |
| Should the trapped desk and military assessment be mandatory evidence of surviving a hazard? | Assessment 82822 is inside a closeable desk with a one-charge acid opening trap; retrieval, reading, and offering are separate events. | Keep the trap and exact loose-item check as authored. Do not award survival or source credit from opening, reading, or carrying a container. |
| What safety outcome is intended for ladder and flagpole falls during the banner route? | The ladder and midair rooms declare fall chances of 5, 80, and 90 percent, with no-ground sectors; slippers do not grant flight or levitation. | Qualify actor-specific admission, survival, return, and recovery. Do not assume the banner or reward protects the recipient. |
| Should the room’s “son’s wedding” note be corrected to match the Duke’s “daughter’s birthday” reply? | The plot note and accepted Duke response describe different occasions; both are source text around the military warning. | Confirm the intended occasion, then correct only the inconsistent prose in a separate bounded content edit. |
| Should the secondary clothing descriptions be reconciled with displayed materials? | The dossier identifies limited material-description differences in local equipment prototypes, separate from the native recipes. | Review each exact prototype and correct only approved misleading strings; preserve item type, stats, and accepted recipe identity. |
| Should the clockmaker and miller deliveries complete only their item handoffs, or prove repaired/running machinery? | Bell → cog and cog → blade are exact item transactions; neither checks an operating clock or mill state. | Keep the item receipts as written. Add machinery progress only with a real accepted machine-state transition. |
| Should the magician’s level guidance or ambient craft/dialogue topics count as a local lesson? | The magician’s ACT_TEACHER response is shared class-qualified runestone guidance; ambient QST families do not provide local practice or saved skill outcomes. | Keep teaching separate from story receipts. Record a lesson only after an eligible practice and durable skill change. |
| Which of the six potential daily candidates should be enabled after active qualification? | Only six item outcomes are marked potential daily; zone resets are mode two, several inputs and recipients have one-copy caps, and supply/renewal is unplayed. | Keep all six as candidates until stock, recipient renewal, accounting settlement, and repeat journeys are qualified. Do not promise fresh sources from reset declarations. |

## Capability additions and balanced repair proposals

| Finding | Plan or proposed repair | Evidence required before calling it complete |
| --- | --- | --- |
| Four paid services are the sole local textile producer route | Add atomic item + coin + output settlement with source-owned accepted receipts; retain the accounting guard until qualified | Each fee, insufficient funds, wrong/bagged/worn input, failed output allocation, replay, save/recovery and no partial consumption; supplied-banner route remains independent |
| Narrative states exceed accepted recipes | Add explicit optional actor/world events only if builders want running clock/mill, hung banner, allegiance or a resolved invasion; choose an AND campaign and branch policy separately | A committed state transition and durable event identity, restart and alternative supplied-material journeys; item acceptance alone awards only the mapped delivery |
| First personal recovery is different from supplied material | Extend provenance for carried/equipped NPC sources, followers, container contents, producer outputs and player transfers | Actual source issuance, ownership transfer, container ancestry, first acquisition and replay; no inferred kills or producer history requirement |
| Flagpole and trapped desk can interrupt an interaction | Add successful access, survival and return evidence when those become optional campaign objectives | Actor alive/removed/relocated cases, protected and unprotected falls, trap spending, container retrieval and restart; preserve ordinary supplied deliveries |
| Note says son's wedding; Duke reply says daughter's birthday | Builder-selected narrow prose correction, retaining exact recipes, rewards and fees | Decide intended occasion; inspect note extra description and accepted reply together, then focused before/after test and separate fix/news entry |
| Merchant/craft/lodging descriptions advertise services with no local binding | Decide which occupations are intentional scenery; restore only selected typed stock, shop/payment or inn bindings | Appropriate goods/keeper/rent identity, guarded settlement and actual player journey; no invented stock or price list from prose |
| Sheep, servant and bellfounder hold requested materials without a local noncombat handover | Decide whether acquisition is intentionally theft/combat/supplied or needs explicit borrowing, sale, commission or shearing | A source-owned accepted handover, accounting transaction when paid, limited issuance and supply/replay tests; journal hints do not implement it |
| Secondary clothing descriptions differ from their displayed materials | Review the limited prototype text discrepancies with builders; correct only selected misleading text | Keep stat/type/value intent explicit, verify all affected descriptions and describe actual changes separately from journal additions |

The wedding/birthday mismatch is concrete source evidence. Decorative shops
and missing peaceful handovers may be intentional zone design, so these are
bounded builder decisions, not assertions that every profession is broken.
Existing one-copy sources can support native offerings but do not establish a
fair personal acquisition route or repeated daily supply. No balance changes,
free materials, invented merchants, world links or disabled guards are needed
to author this source map.

Every later native/content repair should use a separate clearly named `fix`
commit where practical and appear prominently in PR/news with the zone,
player trigger, before/after behavior, validation and limitations. A future
occasion-text repair could say that the Duke's quest reply now matches the
plot note **only after that fix ships**. This checkpoint adds journal guidance
and pending repair plans; it supplies no such native news sentence.

## Qualification and continuation

Existing focused production/native fixtures verify all eleven exact bindings,
nine addressed families, six outcomes/five services, 19 contacts and seventeen
optional checks; current exact material readiness, wrong/equipped alternatives,
supplied two-item evidence, encountered visibility and read-only rendering;
service history without credit and independent receipt recovery. These fixtures
do not certify live shearing, theft, trap survival, flight, fee settlement,
reset supply, chess, a working mill or an invasion.

All 2668 native definitions, content revision two, fingerprint and zone registry
remain unchanged, as do the other sixty story maps. Replacing eleven fallback
rows with six outcome/five service rows leaves 2225 projected rows and 1496
potential daily units; achievement units fall from 1707 to 1702 by correctly
removing the five paid services. There are now 61 authored maps. The original
220-zone queue remains intact: priorities one through forty are source-comprehensive,
180 remain pending. Continue with Home of the Divine (`divhome`).

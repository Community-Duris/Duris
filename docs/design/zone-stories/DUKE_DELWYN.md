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
records the contracts, addressed families, materials and literal assignments.

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
